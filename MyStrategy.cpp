#include "MyStrategy.hpp"
#include <exception>
#include <iostream>
#include <string>
#include <typeinfo>

using std::cout;
using std::endl;
using std::to_string;
using std::vector;

MyStrategy::MyStrategy() {}

Action MyStrategy::getAction(const PlayerView playerView, DebugInterface* debugInterface)
{
    this->debugData.clear();
    this->debugData.emplace_back(new DebugData::PlacedText(ColoredVertex(debug::info_pos, 
                                                                         Vec2Float(0, 0), colors::white), 
                                                                         "Map size: "+to_string(playerView.mapSize), 
                                                                         0, 20));
    this->debugData.emplace_back(new DebugData::PlacedText(ColoredVertex(debug::info_pos, 
                                                                         Vec2Float(0, -20), colors::white), 
                                                                         "My ID: "+to_string(playerView.myId), 
                                                                      0, 20));
    setGlobals(playerView);
    std::unordered_map<int, Player> playersInfo;
    std::unordered_map<int, PlayerPopulation> playerPopulation;
    for (Player player : playerView.players)
    {
        PlayerPopulation population{0, 0};
        playersInfo[player.id] = player;
        playerPopulation[player.id] = population;
    }
        
    
    this->myAvailableResources = playersInfo[playerView.myId].resource;
    std::unordered_map<int, EntityAction> orders{};
    int map [playerView.mapSize][playerView.mapSize];
    for (int i = 0; i < playerView.mapSize; i++)
        for (int j = 0; j < playerView.mapSize; j++)
            map[i][j] = EMPTY_OR_UNKNOWN;
    
    vector<Entity> resourses, 
                   enemyBuilderUnits, enemyAtackUnits, enemyBuildings, enemyHouses,
                   myBuiderUnits, myAtackUnits, myBuildings, myHouses;

    // Fill map and unit arrays.
    for (Entity entity : playerView.entities)
    {
        int entitySize = playerView.entityProperties.at(entity.entityType).size;
        for (int i=entity.position.x; i < entity.position.x + entitySize; i++)
            for (int j=entity.position.x; j < entity.position.x + entitySize; j++)
                map[i][j] = entity.entityType;
        
        if (entity.playerId == nullptr)
        {
            resourses.emplace_back(entity);
        }
        else if (*entity.playerId != playerView.myId)
        {
            playerPopulation[*entity.playerId].available += playerView.entityProperties.at(entity.entityType).populationProvide;
            playerPopulation[*entity.playerId].inUse += playerView.entityProperties.at(entity.entityType).populationUse;
            switch (entity.entityType)
            {
            case EntityType::BUILDER_UNIT:
                enemyBuilderUnits.emplace_back(entity);
                break;
            case EntityType::RANGED_UNIT:
                enemyAtackUnits.emplace_back(entity);
                break;
            case EntityType::MELEE_UNIT:
                enemyAtackUnits.emplace_back(entity);
                break;
            case EntityType::TURRET:
                enemyAtackUnits.emplace_back(entity);
                break;
            case EntityType::BUILDER_BASE:
                enemyBuildings.emplace_back(entity);
                break;
            case EntityType::MELEE_BASE:
                enemyBuildings.emplace_back(entity);
                break;
            case EntityType::RANGED_BASE:
                enemyBuildings.emplace_back(entity);
                break;
            case EntityType::HOUSE:
                enemyHouses.emplace_back(entity);
                break;
            case EntityType::WALL:
                enemyHouses.emplace_back(entity);
                break;
            default:
                break;
            }
        }
        else
        {
            playerPopulation[*entity.playerId].available += playerView.entityProperties.at(entity.entityType).populationProvide;
            playerPopulation[*entity.playerId].inUse += playerView.entityProperties.at(entity.entityType).populationUse;
            switch (entity.entityType)
            {
            case EntityType::BUILDER_UNIT:
                myBuiderUnits.emplace_back(entity);
                break;
            case EntityType::RANGED_UNIT:
                myAtackUnits.emplace_back(entity);
                break;
            case EntityType::MELEE_UNIT:
                myAtackUnits.emplace_back(entity);
                break;
            case EntityType::TURRET:
                myAtackUnits.emplace_back(entity);
                break;
            case EntityType::BUILDER_BASE:
                myBuildings.emplace_back(entity);
                break;
            case EntityType::MELEE_BASE:
                myBuildings.emplace_back(entity);
                break;
            case EntityType::RANGED_BASE:
                myBuildings.emplace_back(entity);
                break;
            case EntityType::HOUSE:
                myHouses.emplace_back(entity);
                break;
            case EntityType::WALL:
                myHouses.emplace_back(entity);
                break;
            default:
                break;
            }
        }
    }

    this->myAvailableResources = playersInfo[playerView.myId].resource;
    this->myAvailablePopulation = playerPopulation[playerView.myId].available - playerPopulation[playerView.myId].inUse;

    for (Entity entity : myBuiderUnits)
    {
        EntityAction action = chooseBuilderUnitAction(entity, playerView, resourses);
        orders[entity.id] = action;
    }

    // std::cout << "map size = " << playerView.mapSize << std::endl;
    // std::cout << "entities = " << playerView.entities.size() << std::endl;
    // std::cout << "float = " << sizeof(float) << std::endl;
    return Action(orders);
}

void MyStrategy::debugUpdate(const PlayerView& playerView, DebugInterface& debugInterface)
{
    debugInterface.send(DebugCommand::Clear());
    for (auto item : this->debugData)
    {
        debugInterface.send(DebugCommand::Add(item));
    }
    debugInterface.getState();

    // ColoredVertex A{std::make_shared<Vec2Float>(10, 10), {0, 0}, colors::red};
    // ColoredVertex B{std::make_shared<Vec2Float>(100, 10), {0, 0}, colors::red};
    // ColoredVertex C{std::make_shared<Vec2Float>(50, 100), {0, 0}, colors::red};
    // std::vector<ColoredVertex> triangle{A, B, C};
    // auto debugTriangle = std::make_shared<DebugData::Primitives>(triangle, PrimitiveType::TRIANGLES);
}

void MyStrategy::setGlobals(const PlayerView& playerView)
{
    this->isGlobalsSet = true;
    for (std::pair<EntityType, EntityProperties> element : playerView.entityProperties)
    {
        this->entityProperties[element.first] = element.second;
    }
}

EntityAction MyStrategy::chooseBuilderUnitAction(Entity& entity, const PlayerView& playerView, vector<Entity>& resoures)
{
    EntityAction resultAction;
    Entity nearestResource = findNearestEntity(entity, resoures);
    MoveAction action;
    action.target = nearestResource.position;
    action.breakThrough = true;
    action.findClosestPosition = true;

    ColoredVertex A{std::make_shared<Vec2Float>(entity.position.x, entity.position.y), {0, 0}, colors::red};
    ColoredVertex B{std::make_shared<Vec2Float>(nearestResource.position.x, nearestResource.position.y), {0, 0}, colors::red};
    // ColoredVertex C{std::make_shared<Vec2Float>(50, 100), {0, 0}, colors::red};
    std::vector<ColoredVertex> line{A, B};
    this->debugData.emplace_back(new DebugData::Primitives(line, PrimitiveType::LINES));
    // auto debugTriangle = std::make_shared<DebugData::Primitives>(triangle, PrimitiveType::TRIANGLES);

    resultAction.moveAction = std::make_shared<MoveAction>(action);
    return resultAction;
}

Entity MyStrategy::findNearestEntity(Entity& entity, std::vector<Entity>& entities)
{
    int min_dist = 100000;
    Entity nearestEntity;
    for (Entity e : entities)
    {
        int d = distance(entity, e, this->entityProperties);
        if (d < min_dist)
        {
            min_dist = d;
            nearestEntity = e;
        }
    }
    return nearestEntity;
}