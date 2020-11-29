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

    std::unordered_map<int, EntityAction> orders{};
    int map [playerView.mapSize][playerView.mapSize];
    for (int i = 0; i < playerView.mapSize; i++)
        for (int j = 0; j < playerView.mapSize; j++)
            map[i][j] = EMPTY_OR_UNKNOWN;
    
    vector<Entity> resourses, 
                   enemyBuilderUnits, enemyAtackUnits, enemyBuildings, enemyHouses,
                   myBuiderUnits, myAtackUnits, myBuildings, myHouses;

    
    for (Entity entity : playerView.entities)
    {
        if (entity.playerId == nullptr)
        {
            map[entity.position.x][entity.position.y] = EntityType::RESOURCE;
            resourses.emplace_back(entity);
        }
        else if (*entity.playerId != playerView.myId)
        {
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
        
        // if (entity.entityType != EntityType::RESOURCE)
        // {
        //     std::cout << *entity.playerId << " ";
        // }
        // std::cout << *entity.playerId << " ";
        // if (entity.playerId == this->myId)
        // {
            // std::cout << "my unit" << std::endl;
            // switch (entity.entityType)
            // {
            // case EntityType::BUILDER_UNIT:
            //     std::cout << "builder unit (" << entity.position.x << ", " << entity.position.y << ")" << std::endl;
            //     break;
            
            // default:
            //     break;
            // }
        // }
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
}
