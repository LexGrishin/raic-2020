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
    
    resourses.clear(); 
    enemyBuilderUnits.clear();
    enemyAtackUnits.clear();
    enemyBuildings.clear();
    enemyHouses.clear();
    myBuiderUnits.clear();
    myAtackUnits.clear();
    myBuildings.clear();
    myHouses.clear();
    builderUnitOrder = 0;
    atackUnitOrder = 0;

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
        EntityAction action = chooseBuilderUnitAction(entity, playerView);
        orders[entity.id] = action;
    }

    for (Entity entity : myBuildings)
    {
        EntityAction action = chooseRecruitUnitAction(entity, playerView);
        orders[entity.id] = action;
    }

    for (Entity entity : myAtackUnits)
    {
        EntityAction action = chooseAtackUnitAction(entity, playerView);
        orders[entity.id] = action;
    }

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


int MyStrategy::distance(Entity& e1, Entity& e2)
{
    int dx, dy;
    int e1_xmin = e1.position.x;
    int e1_xmax = e1.position.x + entityProperties.at(e1.entityType).size - 1;
    int e1_ymin = e1.position.y;
    int e1_ymax = e1.position.y + entityProperties.at(e1.entityType).size - 1;
    int e2_xmin = e2.position.x;
    int e2_xmax = e2.position.x + entityProperties.at(e2.entityType).size - 1;
    int e2_ymin = e2.position.y;
    int e2_ymax = e2.position.y + entityProperties.at(e2.entityType).size - 1;

    if ((e1_xmin >= e2_xmin && e1_xmin <= e2_xmax)
     || (e1_xmax >= e2_xmin && e1_xmax <= e2_xmax)
     || (e2_xmin >= e1_xmin && e2_xmin <= e1_xmax)
     || (e2_xmax >= e1_xmin && e2_xmax <= e1_xmax))
        dx = 0;
    else if (e1_xmin < e2_xmin)
        dx = e2_xmin - e1_xmin - entityProperties.at(e1.entityType).size + 1;
    else
        dx = e1_xmin - e2_xmin - entityProperties.at(e2.entityType).size + 1;

    if ((e1_ymin >= e2_ymin && e1_ymin <= e2_ymax)
     || (e1_ymax >= e2_ymin && e1_ymax <= e2_ymax)
     || (e2_ymin >= e1_ymin && e2_ymin <= e1_ymax)
     || (e2_ymax >= e1_ymin && e2_ymax <= e1_ymax))
        dy = 0;
    else if (e1_ymin < e2_ymin)
        dy = e2_ymin - e1_ymin - entityProperties.at(e1.entityType).size + 1;
    else
        dy = e1_ymin - e2_ymin - entityProperties.at(e2.entityType).size + 1;
    return dx + dy - 1;
}

EntityAction MyStrategy::chooseBuilderUnitAction(Entity& entity, const PlayerView& playerView)
{
    EntityAction resultAction;
    Entity nearestResource = findNearestEntity(entity, resourses);
    Entity nearestEnemyAtackUnit = findNearestEntity(entity, enemyAtackUnits);
    Entity nearestEnemyBuilderUnit = findNearestEntity(entity, enemyBuilderUnits);
    if (distance(entity, nearestEnemyAtackUnit)<=(*entityProperties[nearestEnemyAtackUnit.entityType].attack).attackRange)
    {
        AttackAction action;
        action.target = std::make_shared<int>(nearestEnemyAtackUnit.id);
        resultAction.attackAction = std::make_shared<AttackAction>(action);
    }
    else if (distance(entity, nearestEnemyBuilderUnit)<=(*entityProperties[nearestEnemyBuilderUnit.entityType].attack).attackRange)
    {
        AttackAction action;
        action.target = std::make_shared<int>(nearestEnemyBuilderUnit.id);
        resultAction.attackAction = std::make_shared<AttackAction>(action);
    }
    else
    {
        MoveAction action;
        action.target = nearestResource.position;
        action.breakThrough = true;
        action.findClosestPosition = true;

        ColoredVertex A{std::make_shared<Vec2Float>(entity.position.x, entity.position.y), {0, 0}, colors::red};
        ColoredVertex B{std::make_shared<Vec2Float>(nearestResource.position.x, nearestResource.position.y), {0, 0}, colors::red};
        // ColoredVertex C{std::make_shared<Vec2Float>(50, 100), {0, 0}, colors::red};
        std::vector<ColoredVertex> line{A, B};
        this->debugData.emplace_back(new DebugData::Primitives(line, PrimitiveType::LINES));

        resultAction.moveAction = std::make_shared<MoveAction>(action);
    }
    
    return resultAction;
}

EntityAction MyStrategy::chooseRecruitUnitAction(Entity& entity, const PlayerView& playerView)
{
    EntityAction resultAction;
    float currentBalance = (myBuiderUnits.size() + builderUnitOrder)/(myAtackUnits.size() + atackUnitOrder + myBuiderUnits.size() + builderUnitOrder);
    if (entity.entityType == EntityType::BUILDER_BASE)
    {
        if (currentBalance < unitBalance(playerView.currentTick))
        {
            BuildAction action;
            action.entityType = EntityType::BUILDER_UNIT;
            action.position = {entity.position.x, entity.position.y - 1};
            resultAction.buildAction = std::make_shared<BuildAction>(action);
            builderUnitOrder += 1;
        }
        else resultAction.buildAction = nullptr;
    }
    else if (entity.entityType == EntityType::RANGED_BASE)
    {
        if (currentBalance >= unitBalance(playerView.currentTick))
        {
            BuildAction action;
            action.entityType = EntityType::RANGED_UNIT;
            action.position = {entity.position.x, entity.position.y - 1};
            resultAction.buildAction = std::make_shared<BuildAction>(action);
            atackUnitOrder += 1;
        }
        else resultAction.buildAction = nullptr;
    }
    else if (entity.entityType == EntityType::MELEE_BASE)
    {
        if (currentBalance >= unitBalance(playerView.currentTick))
        {
            BuildAction action;
            action.entityType = EntityType::MELEE_UNIT;
            action.position = {entity.position.x, entity.position.y - 1};
            resultAction.buildAction = std::make_shared<BuildAction>(action);
            atackUnitOrder += 1;
        }
        else resultAction.buildAction = nullptr;
    }
    else resultAction.buildAction = nullptr;
    return resultAction;
}

EntityAction MyStrategy::chooseAtackUnitAction(Entity& entity, const PlayerView& playerView)
{
    EntityAction resultAction;
    AttackAction action;
    AutoAttack autoAttack;
    autoAttack.pathfindRange = 40;
    autoAttack.validTargets = {EntityType::RANGED_UNIT,
                               EntityType::MELEE_UNIT,
                               EntityType::TURRET,
                               EntityType::BUILDER_UNIT,
                               EntityType::RANGED_BASE,
                               EntityType::MELEE_BASE,
                               EntityType::BUILDER_BASE,
                               EntityType::HOUSE
                              };
    action.autoAttack = std::make_shared<AutoAttack>(autoAttack);
    resultAction.attackAction = std::make_shared<AttackAction>(action);
    return resultAction;
}

Entity MyStrategy::findNearestEntity(Entity& entity, std::vector<Entity>& entities)
{
    int min_dist = 100000;
    Entity nearestEntity;
    for (Entity e : entities)
    {
        int d = distance(entity, e);
        if (d < min_dist)
        {
            min_dist = d;
            nearestEntity = e;
        }
    }
    return nearestEntity;
}