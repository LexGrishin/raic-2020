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
    
    vector<vector<int>> mapOccupied(playerView.mapSize, vector<int>(playerView.mapSize, 0));
    vector<vector<int>> mapBuilding(playerView.mapSize, vector<int>(playerView.mapSize, 0));
    vector<vector<int>> mapAttack(playerView.mapSize, vector<int>(playerView.mapSize, 0));
    vector<vector<int>> mapDamage(playerView.mapSize, vector<int>(playerView.mapSize, 0));
          
    resourses.clear(); 
    availableResourses.clear();
    myBuiderUnits.clear();
    myAttackUnits.clear();
    myTurrets.clear();
    myBuildings.clear();
    myDamagedBuildings.clear();
    possibleBuildPositions.clear();
    enemyEntities.clear();
    
    builderUnitOrder = 0;
    atackUnitOrder = 0;
    constructOrder = 0;

    // Fill maps and unit arrays.
    for (Entity entity : playerView.entities)
    {
        int entitySize = entityProperties[entity.entityType].size;
        fillMapCells(mapOccupied, entity.position, entitySize, 0);
        if ((entity.entityType == EntityType::HOUSE)
            ||(entity.entityType == EntityType::BUILDER_BASE) 
            ||(entity.entityType == EntityType::MELEE_BASE)
            ||(entity.entityType == EntityType::RANGED_BASE)
            ||(entity.entityType == EntityType::TURRET)
            ||(entity.entityType == EntityType::RESOURCE)
            ||(entity.entityType == EntityType::WALL)
           )
           fillMapCells(mapBuilding, entity.position, entitySize, 1);
        
        if (entity.playerId == nullptr)
        {
            resourses.emplace_back(entity);
        }
        else if (*entity.playerId != playerView.myId)
        {
            if (entity.active)
            {
                playerPopulation[*entity.playerId].available += entityProperties[entity.entityType].populationProvide;
                playerPopulation[*entity.playerId].inUse += entityProperties[entity.entityType].populationUse;
            }
            enemyEntities.emplace_back(entity);
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
                myAttackUnits.emplace_back(entity);
                break;
            case EntityType::MELEE_UNIT:
                myAttackUnits.emplace_back(entity);
                break;
            case EntityType::TURRET:
                if (entity.health < entityProperties[entity.entityType].maxHealth)
                    myDamagedBuildings.emplace_back(entity);
                if (entity.active == true)
                    myTurrets.emplace_back(entity);
                break;
            case EntityType::BUILDER_BASE:
                if (entity.health < entityProperties[entity.entityType].maxHealth)
                    myDamagedBuildings.emplace_back(entity);
                if (entity.active == true)
                    myBuildings.emplace_back(entity);
                break;
            case EntityType::MELEE_BASE:
                if (entity.health < entityProperties[entity.entityType].maxHealth)
                    myDamagedBuildings.emplace_back(entity);
                if (entity.active == true)
                    myBuildings.emplace_back(entity);
                break;
            case EntityType::RANGED_BASE:
                if (entity.health < entityProperties[entity.entityType].maxHealth)
                    myDamagedBuildings.emplace_back(entity);
                if (entity.active == true)
                    myBuildings.emplace_back(entity);
                break;
            case EntityType::HOUSE:
                if (entity.health < entityProperties[entity.entityType].maxHealth)
                    myDamagedBuildings.emplace_back(entity);
                break;
            case EntityType::WALL:
                if (entity.health < entityProperties[entity.entityType].maxHealth)
                    myDamagedBuildings.emplace_back(entity);
                break;
            default:
                break;
            }
        }
    }

    for (Entity res : resourses)
    {
        if (isAvailable(mapOccupied, res.position, entityProperties[EntityType::RESOURCE].size))
            availableResourses.emplace_back(res);
    }

    for (Entity ent: enemyEntities)
    {
        if ((ent.entityType == EntityType::RANGED_UNIT)
            || (ent.entityType == EntityType::MELEE_UNIT)
            || (ent.entityType == EntityType::TURRET)
            || (ent.entityType == EntityType::BUILDER_UNIT))
        {
            int radius = (*entityProperties[ent.entityType].attack).attackRange;
            int damage = (*entityProperties[ent.entityType].attack).damage;
            fillDamageMap(mapDamage, ent.position, radius, damage);
        }
    }

    for (Entity ent: myAttackUnits)
    {
        int radius = (*entityProperties[ent.entityType].attack).attackRange;
        int damage = (*entityProperties[ent.entityType].attack).damage;
        fillDamageMap(mapAttack, ent.position, radius, damage);
    }

    for (Entity ent: myTurrets) // TODO: Исправить расчет поля атаки турели с учетом ее размера!
    {
        int radius = (*entityProperties[ent.entityType].attack).attackRange;
        int damage = (*entityProperties[ent.entityType].attack).damage;
        fillDamageMap(mapAttack, ent.position, radius, damage);
    }

    possibleBuildPositions = findFreePosOnBuildCellMap(mapBuilding, entityProperties[EntityType::HOUSE].size);
    this->myAvailableResources = playersInfo[playerView.myId].resource;
    this->myAvailablePopulation = playerPopulation[playerView.myId].available - playerPopulation[playerView.myId].inUse;

    for (Entity entity : myBuiderUnits)
    {
        EntityAction action = chooseBuilderUnitAction(entity, playerView, mapOccupied);
        orders[entity.id] = action;
    }

    for (Entity entity : myBuildings)
    {
        EntityAction action = chooseRecruitUnitAction(entity, playerView);
        orders[entity.id] = action;
    }

    for (Entity entity : myAttackUnits)
    {
        EntityAction action = chooseAtackUnitAction(entity, playerView, mapAttack, mapDamage);
        orders[entity.id] = action;
    }

    for (Entity entity : myTurrets)
    {
        EntityAction action = chooseAtackUnitAction(entity, playerView, mapAttack, mapDamage);
        orders[entity.id] = action;
    }

    // for (int i=0; i < mapDamage[0].size(); i++)
    //     for (int j=0; j < mapDamage[0].size(); j++)
    //     {
    //         if (mapDamage[i][j] != 0)
    //         {
    //             this->debugData.emplace_back(new DebugData::PlacedText(ColoredVertex(std::make_shared<Vec2Float>(i, j), 
    //                                                                      Vec2Float(0, 0), colors::red), 
    //                                                                      to_string(mapDamage[i][j]), 
    //                                                                      0, 10));
    //         }
    //     }

    float tickBalance = unitBalance(playerView.currentTick);
    float currentBalance = float(myBuiderUnits.size())/float(myAttackUnits.size() + myBuiderUnits.size());
    // this->debugData.emplace_back(new DebugData::PlacedText(ColoredVertex(debug::info_pos, 
    //                                                                      Vec2Float(0, 0), colors::white), 
    //                                                                      "Current balance: "+to_string(currentBalance)+" / "+to_string(tickBalance), 
    //                                                                      0, 20));

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
    Entity nearestEnemy = findNearestEntity(entity, enemyEntities, map, false);
    Entity nearestDamagedBuilding = findNearestEntity(entity, myDamagedBuildings, map, false);
    Entity nearestBuildPosition = findNearestEntity(entity, possibleBuildPositions, map, false);

    if ((nearestEnemy.id != -1) && (distance(entity, nearestEnemy)<(*entityProperties[entity.entityType].attack).attackRange))
    {
        AttackAction action;
        action.target = std::make_shared<int>(nearestEnemy.id);
        resultAction.attackAction = std::make_shared<AttackAction>(action);
    }
    else if ((nearestDamagedBuilding.id != -1) && (distance(entity, nearestDamagedBuilding) > 0) && (distance(entity, nearestDamagedBuilding) < 2))
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
    else if ((myAvailablePopulation < 10) && (constructOrder < 3) && (myAvailableResources >= 50))
    {
        constructOrder += 1;
        myAvailableResources -= entityProperties[EntityType::HOUSE].cost;
        if (distance(entity, nearestBuildPosition) != 0)
        {
            MoveAction action;
            action.target = nearestBuildPosition.position;
            action.breakThrough = false;
            action.findClosestPosition = true;
            resultAction.moveAction = std::make_shared<MoveAction>(action);
        }
        else
        {
            BuildAction action;
            action.entityType = EntityType::HOUSE;
            action.position = nearestBuildPosition.position;
            resultAction.buildAction = std::make_shared<BuildAction>(action);
        }

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
    else if (nearestEnemy.id != -1)
    {
        MoveAction action;
        action.target = nearestEnemy.position;
        action.breakThrough = false;
        action.findClosestPosition = true;
        resultAction.moveAction = std::make_shared<MoveAction>(action);

        ColoredVertex A{std::make_shared<Vec2Float>(entity.position.x, entity.position.y), {0, 0}, colors::red};
        ColoredVertex B{std::make_shared<Vec2Float>(nearestEnemy.position.x, nearestEnemy.position.y), {0, 0}, colors::red};
        std::vector<ColoredVertex> line{A, B};
        this->debugData.emplace_back(new DebugData::Primitives(line, PrimitiveType::LINES));
    }
    return resultAction;
}

EntityAction MyStrategy::chooseRecruitUnitAction(Entity& entity, const PlayerView& playerView)
{
    EntityAction resultAction;
    float tickBalance = unitBalance(playerView.currentTick);
    float currentBalance = float(myBuiderUnits.size() + builderUnitOrder)/float(myAttackUnits.size() + atackUnitOrder + myBuiderUnits.size() + builderUnitOrder);
    bool reqruitBuilder = false;
    if (myAvailableResources - lastAvailableResources < 20)
       reqruitBuilder = true; 
    if (entity.entityType == EntityType::BUILDER_BASE)
    {
        if ((currentBalance < tickBalance) || (reqruitBuilder == true))
        {
            BuildAction action;
            action.entityType = EntityType::BUILDER_UNIT;
            action.position = {entity.position.x + entityProperties[entity.entityType].size, 
                               entity.position.y + entityProperties[entity.entityType].size - 1};
            resultAction.buildAction = std::make_shared<BuildAction>(action);
            builderUnitOrder += 1;
        }
        else resultAction.buildAction = nullptr;
    }
    else if (entity.entityType == EntityType::RANGED_BASE)
    {
        if ((currentBalance >= tickBalance))
        {
            BuildAction action;
            action.entityType = EntityType::RANGED_UNIT;
            action.position = {entity.position.x + entityProperties[entity.entityType].size, 
                               entity.position.y + entityProperties[entity.entityType].size - 1};
            resultAction.buildAction = std::make_shared<BuildAction>(action);
            atackUnitOrder += 1;
        }
        else resultAction.buildAction = nullptr;
    }
    else resultAction.buildAction = nullptr;
    return resultAction;
}

EntityAction MyStrategy::chooseAtackUnitAction(Entity& entity, const PlayerView& playerView, vector<vector<int>>& mapAttack, vector<vector<int>>& mapDamage)
{
    EntityAction resultAction;
    bool ignoreAvailable = false;
    if (entity.entityType == EntityType::RANGED_UNIT || entity.entityType == EntityType::TURRET)
        ignoreAvailable = true;
    
    Entity nearestEnemy = findNearestEntity(entity, enemyEntities, mapAttack, ignoreAvailable);

    if (nearestEnemy.id != -1)
    {
        if (distance(entity, nearestEnemy)>=(*entityProperties[entity.entityType].attack).attackRange)
        {
            MoveAction action;
            action.target = nearestEnemy.position;
            action.breakThrough = false;
            action.findClosestPosition = true;
            resultAction.moveAction = std::make_shared<MoveAction>(action);
        }
        else
        {
            AttackAction action;
            action.target = std::make_shared<int>(nearestEnemy.id);
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

// ConstructAction MyStrategy::constructHouse(Vec2Int buildingPosition, vector<vector<int>>& map)
// {
//     ConstructAction constructAction;
//     constructAction.entityId = -1;
    
//     Entity buildPos;
//     buildPos.position = buildingPosition;
//     buildPos.position.x += entityProperties[EntityType::HOUSE].size;
//     buildPos.entityType = EntityType::BUILDER_UNIT;
//     Entity nearestBuilderUnit = myBuiderUnits[0]; // findNearestEntity(buildPos, myBuiderUnits, map, false);
//     if (nearestBuilderUnit.id != -1)
//     {
//         int dist = distance(buildPos, nearestBuilderUnit);
//         if (dist >= 0)
//         {
//             MoveAction action;
//             action.target = buildPos.position;
//             action.breakThrough = false;
//             action.findClosestPosition = true;
//             constructAction.entityAction.moveAction = std::make_shared<MoveAction>(action);
//             constructAction.entityId = nearestBuilderUnit.id;

//             ColoredVertex A{std::make_shared<Vec2Float>(buildPos.position.x, buildPos.position.y), {0, 0}, colors::green};
//             ColoredVertex B{std::make_shared<Vec2Float>(nearestBuilderUnit.position.x, nearestBuilderUnit.position.y), {0, 0}, colors::green};
//             std::vector<ColoredVertex> line{A, B};
//             this->debugData.emplace_back(new DebugData::Primitives(line, PrimitiveType::LINES));
//         }
//         else
//         {
//             BuildAction action;
//             action.entityType = EntityType::HOUSE;
//             action.position = buildingPosition;
//             constructAction.entityAction.buildAction = std::make_shared<BuildAction>(action);
//             constructAction.entityId = nearestBuilderUnit.id;
//         }
//     }
    
//     return constructAction;
// }

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