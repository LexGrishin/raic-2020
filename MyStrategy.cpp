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
    std::unordered_map<int, EntityAction> orders{};

    this->debugData.clear();

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
    
    vector<vector<int>> occupiedCellsMap(playerView.mapSize, vector<int>(playerView.mapSize, 0));
    vector<vector<int>> buildCellsMap(playerView.mapSize, vector<int>(playerView.mapSize, 0));
    vector<vector<int>> avoidZonesMap(playerView.mapSize, vector<int>(playerView.mapSize, 0));
    vector<vector<int>> attackMap(playerView.mapSize, vector<int>(playerView.mapSize, 0));
    
      
    resourses.clear(); 
    reachableResourses.clear();
    enemyBuilderUnits.clear();
    enemyAtackUnits.clear();
    enemyBuildings.clear();
    enemyHouses.clear();
    myBuiderUnits.clear();
    myAtackUnits.clear();
    myBuildings.clear();
    myHouses.clear();
    myDamagedBuildings.clear();
    builderUnitOrder = 0;
    atackUnitOrder = 0;

    // Fill maps and unit arrays.
    for (Entity entity : playerView.entities)
    {
        int entitySize = entityProperties[entity.entityType].size;
        fillMapCells(occupiedCellsMap, entity.position, entitySize, 0);
        if ((entity.entityType == EntityType::HOUSE)
            ||(entity.entityType == EntityType::BUILDER_BASE) 
            ||(entity.entityType == EntityType::MELEE_BASE)
            ||(entity.entityType == EntityType::RANGED_BASE)
            ||(entity.entityType == EntityType::TURRET)
           )
           fillMapCells(buildCellsMap, entity.position, entitySize, 1);
                
        if (entity.playerId == nullptr)
        {
            resourses.emplace_back(entity);
        }
        else if (*entity.playerId != playerView.myId)
        {
            playerPopulation[*entity.playerId].available += entityProperties[entity.entityType].populationProvide;
            playerPopulation[*entity.playerId].inUse += entityProperties[entity.entityType].populationUse;
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
            playerPopulation[*entity.playerId].available += entityProperties[entity.entityType].populationProvide;
            playerPopulation[*entity.playerId].inUse += entityProperties[entity.entityType].populationUse;
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
                if (entity.health < entityProperties[entity.entityType].maxHealth)
                    myDamagedBuildings.emplace_back(entity);
                else
                    myAtackUnits.emplace_back(entity);
                break;
            case EntityType::BUILDER_BASE:
                if (entity.health < entityProperties[entity.entityType].maxHealth)
                    myDamagedBuildings.emplace_back(entity);
                else
                    myBuildings.emplace_back(entity);
                break;
            case EntityType::MELEE_BASE:
                if (entity.health < entityProperties[entity.entityType].maxHealth)
                    myDamagedBuildings.emplace_back(entity);
                else
                    myBuildings.emplace_back(entity);
                break;
            case EntityType::RANGED_BASE:
                if (entity.health < entityProperties[entity.entityType].maxHealth)
                    myDamagedBuildings.emplace_back(entity);
                else
                    myBuildings.emplace_back(entity);
                break;
            case EntityType::HOUSE:
                if (entity.health < entityProperties[entity.entityType].maxHealth)
                    myDamagedBuildings.emplace_back(entity);
                else
                    myHouses.emplace_back(entity);
                break;
            case EntityType::WALL:
                if (entity.health < entityProperties[entity.entityType].maxHealth)
                    myDamagedBuildings.emplace_back(entity);
                else
                    myHouses.emplace_back(entity);
                break;
            default:
                break;
            }
        }
    }

    for (Entity res : resourses)
    {
        if (isAvailable(occupiedCellsMap, res.position, entityProperties[EntityType::RESOURCE].size))
            reachableResourses[res.id] = res;
    }

    this->myAvailableResources = playersInfo[playerView.myId].resource;
    this->myAvailablePopulation = playerPopulation[playerView.myId].available - playerPopulation[playerView.myId].inUse;

    for (Entity entity : myBuiderUnits)
    {
        EntityAction action = chooseBuilderUnitAction(entity, playerView, occupiedCellsMap);
        orders[entity.id] = action;
    }

    for (Entity entity : myBuildings)
    {
        EntityAction action = chooseRecruitUnitAction(entity, playerView);
        orders[entity.id] = action;
    }

    for (Entity entity : myAtackUnits)
    {
        EntityAction action = chooseAtackUnitAction(entity, playerView, occupiedCellsMap);
        orders[entity.id] = action;
    }

    float tickBalance = unitBalance(playerView.currentTick);
    float currentBalance = float(myBuiderUnits.size())/float(myAtackUnits.size() + myBuiderUnits.size());
    this->debugData.emplace_back(new DebugData::PlacedText(ColoredVertex(debug::info_pos, 
                                                                         Vec2Float(0, 0), colors::white), 
                                                                         "Current balance: "+to_string(currentBalance)+" / "+to_string(tickBalance), 
                                                                         0, 20));

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

EntityAction MyStrategy::chooseBuilderUnitAction(Entity& entity, const PlayerView& playerView, vector<vector<int>>& map)
{
    EntityAction resultAction;
    Entity nearestResource = findNearestEntity(entity, resourses, map, false);
    Entity nearestEnemyAtackUnit = findNearestEntity(entity, enemyAtackUnits, map, false);
    Entity nearestEnemyBuilderUnit = findNearestEntity(entity, enemyBuilderUnits, map, false);
    Entity nearestDamagedBuilding = findNearestEntity(entity, myDamagedBuildings, map, false);

    if ((nearestEnemyAtackUnit.id != -1) && (distance(entity, nearestEnemyAtackUnit)<(*entityProperties[entity.entityType].attack).attackRange))
    {
        AttackAction action;
        action.target = std::make_shared<int>(nearestEnemyAtackUnit.id);
        resultAction.attackAction = std::make_shared<AttackAction>(action);
    }
    else if ((nearestEnemyBuilderUnit.id != -1) && (distance(entity, nearestEnemyBuilderUnit)<(*entityProperties[entity.entityType].attack).attackRange))
    {
        AttackAction action;
        action.target = std::make_shared<int>(nearestEnemyBuilderUnit.id);
        resultAction.attackAction = std::make_shared<AttackAction>(action);
    }
    else if ((nearestDamagedBuilding.id != -1) && (distance(entity, nearestDamagedBuilding) > 0) && (distance(entity, nearestDamagedBuilding) < 15))
    {
        MoveAction action;
        action.target = nearestDamagedBuilding.position;
        action.breakThrough = false;
        action.findClosestPosition = true;

        ColoredVertex A{std::make_shared<Vec2Float>(entity.position.x, entity.position.y), {0, 0}, colors::red};
        ColoredVertex B{std::make_shared<Vec2Float>(nearestDamagedBuilding.position.x, nearestDamagedBuilding.position.y), {0, 0}, colors::green};
        std::vector<ColoredVertex> line{A, B};
        this->debugData.emplace_back(new DebugData::Primitives(line, PrimitiveType::LINES));

        resultAction.moveAction = std::make_shared<MoveAction>(action);
    }
    else if ((nearestDamagedBuilding.id != -1) && (distance(entity, nearestDamagedBuilding) == 0))
    {
        RepairAction action;
        action.target = nearestDamagedBuilding.id;
        resultAction.repairAction = std::make_shared<RepairAction>(action);
    }
    else if ((nearestResource.id != -1) && (distance(entity, nearestResource)>=(*entityProperties[entity.entityType].attack).attackRange))
    {
        MoveAction action;
        action.target = nearestResource.position;
        action.breakThrough = false;
        action.findClosestPosition = true;

        ColoredVertex A{std::make_shared<Vec2Float>(entity.position.x, entity.position.y), {0, 0}, colors::red};
        ColoredVertex B{std::make_shared<Vec2Float>(nearestResource.position.x, nearestResource.position.y), {0, 0}, colors::red};
        std::vector<ColoredVertex> line{A, B};
        this->debugData.emplace_back(new DebugData::Primitives(line, PrimitiveType::LINES));

        resultAction.moveAction = std::make_shared<MoveAction>(action);
    }
    else if ((nearestResource.id != -1) && (distance(entity, nearestResource)<(*entityProperties[entity.entityType].attack).attackRange))
    {
        AttackAction action;
        action.target = std::make_shared<int>(nearestResource.id);
        resultAction.attackAction = std::make_shared<AttackAction>(action);
    }
    else if (nearestEnemyBuilderUnit.id != -1)
    {
        MoveAction action;
        action.target = nearestEnemyBuilderUnit.position;
        action.breakThrough = false;
        action.findClosestPosition = true;
        resultAction.moveAction = std::make_shared<MoveAction>(action);

        ColoredVertex A{std::make_shared<Vec2Float>(entity.position.x, entity.position.y), {0, 0}, colors::red};
        ColoredVertex B{std::make_shared<Vec2Float>(nearestEnemyBuilderUnit.position.x, nearestEnemyBuilderUnit.position.y), {0, 0}, colors::red};
        std::vector<ColoredVertex> line{A, B};
        this->debugData.emplace_back(new DebugData::Primitives(line, PrimitiveType::LINES));
    }
    else if (nearestEnemyAtackUnit.id != -1)
    {   
        MoveAction action;
        action.target = nearestEnemyAtackUnit.position;
        action.breakThrough = false;
        action.findClosestPosition = true;
        resultAction.moveAction = std::make_shared<MoveAction>(action);

        ColoredVertex A{std::make_shared<Vec2Float>(entity.position.x, entity.position.y), {0, 0}, colors::red};
        ColoredVertex B{std::make_shared<Vec2Float>(nearestEnemyAtackUnit.position.x, nearestEnemyAtackUnit.position.y), {0, 0}, colors::red};
        std::vector<ColoredVertex> line{A, B};
        this->debugData.emplace_back(new DebugData::Primitives(line, PrimitiveType::LINES));
    }
    return resultAction;
}

EntityAction MyStrategy::chooseRecruitUnitAction(Entity& entity, const PlayerView& playerView)
{
    EntityAction resultAction;
    float tickBalance = unitBalance(playerView.currentTick);
    float currentBalance = float(myBuiderUnits.size() + builderUnitOrder)/float(myAtackUnits.size() + atackUnitOrder + myBuiderUnits.size() + builderUnitOrder);

    if (entity.entityType == EntityType::BUILDER_BASE)
    {
        if (currentBalance < tickBalance)
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
        if (currentBalance >= tickBalance)
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
        if (currentBalance >= tickBalance)
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

EntityAction MyStrategy::chooseAtackUnitAction(Entity& entity, const PlayerView& playerView, vector<vector<int>>& map)
{
    EntityAction resultAction;
    bool ignoreAvailable = false;
    if (entity.entityType == EntityType::RANGED_UNIT || entity.entityType == EntityType::TURRET)
        ignoreAvailable = true;
    
    Entity nearestEnemyAttackUnit = findNearestEntity(entity, enemyAtackUnits, map, ignoreAvailable);
    Entity nearestEnemyBuilderUnit = findNearestEntity(entity, enemyBuilderUnits, map, ignoreAvailable);
    Entity nearestEnemyBuilding = findNearestEntity(entity, enemyBuildings, map, ignoreAvailable);
    if (nearestEnemyAttackUnit.id != -1)
    {
        if (distance(entity, nearestEnemyAttackUnit)>=(*entityProperties[entity.entityType].attack).attackRange)
        {
            MoveAction action;
            action.target = nearestEnemyAttackUnit.position;
            action.breakThrough = false;
            action.findClosestPosition = true;
            resultAction.moveAction = std::make_shared<MoveAction>(action);
        }
        else
        {
            AttackAction action;
            action.target = std::make_shared<int>(nearestEnemyAttackUnit.id);
            resultAction.attackAction = std::make_shared<AttackAction>(action);
        }       
    }
    else if (nearestEnemyBuilderUnit.id != -1)
    {
        if (distance(entity, nearestEnemyBuilderUnit)>=(*entityProperties[entity.entityType].attack).attackRange)
        {
            MoveAction action;
            action.target = nearestEnemyBuilderUnit.position;
            action.breakThrough = false;
            action.findClosestPosition = true;
            resultAction.moveAction = std::make_shared<MoveAction>(action);
        }
        else
        {
            AttackAction action;
            action.target = std::make_shared<int>(nearestEnemyBuilderUnit.id);
            resultAction.attackAction = std::make_shared<AttackAction>(action);
        }       
    }
    else if (nearestEnemyBuilding.id != -1)
    {
        if (distance(entity, nearestEnemyBuilding)>=(*entityProperties[entity.entityType].attack).attackRange)
        {
            MoveAction action;
            action.target = nearestEnemyBuilding.position;
            action.breakThrough = false;
            action.findClosestPosition = true;
            resultAction.moveAction = std::make_shared<MoveAction>(action);
        }
        else
        {
            AttackAction action;
            action.target = std::make_shared<int>(nearestEnemyBuilding.id);
            resultAction.attackAction = std::make_shared<AttackAction>(action);
        }       
    }
    else
    {
        AttackAction action;
        AutoAttack autoAttack;
        autoAttack.pathfindRange = 160;
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
    }
    return resultAction;
}

Entity MyStrategy::findNearestEntity(Entity& entity, std::vector<Entity>& entities, vector<vector<int>>& map, bool ignoreAvailable)
{
    int min_dist = 100000;
    Entity nearestEntity;
    nearestEntity.id = -1;
    for (Entity e : entities)
    {
        int d = distance(entity, e);
        if ((d < min_dist) && (isAvailable(map, e.position, entityProperties[e.entityType].size) || d == 0 || ignoreAvailable))
        {
            min_dist = d;
            nearestEntity = e;
        }
    }
    return nearestEntity;
}

Entity MyStrategy::findNearestReachableResource(Entity& entity, std::unordered_map<int, Entity>& entities)
{
    int min_dist = 100000;
    int nearestEntityId = -1;
    Entity nearestEntity;
    nearestEntity.id = -1;
    for (auto& e : entities)
    {
        int d = distance(entity, e.second);
        if (d < min_dist)
        {
            min_dist = d;
            nearestEntity = e.second;
            nearestEntityId = e.first;
        }
    }
    entities.erase(nearestEntityId);
    return nearestEntity;
}