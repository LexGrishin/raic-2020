#include "MyStrategy.hpp"
#include <exception>
#include <iostream>
#include <string>
#include <typeinfo>
#include <cmath>

using std::cout;
using std::endl;
using std::to_string;
using std::vector;

MyStrategy::MyStrategy() {}

Action MyStrategy::getAction(const PlayerView playerView, DebugInterface* debugInterface)
{
    std::unordered_map<int, EntityAction> orders{};

    this->debugData.clear();

    if (isGlobalsSet == false)
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
    // vector<vector<int>> mapAttack(playerView.mapSize, vector<int>(playerView.mapSize, 0));
    vector<vector<int>> mapDamage(playerView.mapSize, vector<int>(playerView.mapSize, 0));
    vector<vector<int>> mapAlly(playerView.mapSize, vector<int>(playerView.mapSize, -1));
    vector<vector<int>> mapEnemy(playerView.mapSize, vector<int>(playerView.mapSize, -1));

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

    countBuilderUnits = 0;
    countMeleeUnits = 0;
    countRangeUnits = 0;


    // Fill maps and unit arrays.
    for (Entity entity : playerView.entities)
    {
        int entitySize = entityProperties[entity.entityType].size;
        int value = 0;
        if (entity.entityType == EntityType::RESOURCE)
            value = 1;
        else if ((entity.entityType == EntityType::BUILDER_UNIT)
                 ||(entity.entityType == EntityType::RANGED_UNIT)
                 ||(entity.entityType == EntityType::MELEE_UNIT)
                )
            value = 2;
        else if ((entity.entityType == EntityType::BUILDER_BASE)
            ||(entity.entityType == EntityType::RANGED_BASE)
            ||(entity.entityType == EntityType::MELEE_BASE)
            ||(entity.entityType == EntityType::HOUSE)
            ||(entity.entityType == EntityType::WALL)
            ||(entity.entityType == EntityType::TURRET))
            value = 3;
        else
            value = 0;
        fillMapCells(mapOccupied, entity.position, value, entitySize, 0);

        if ((entity.entityType == EntityType::HOUSE)
            ||(entity.entityType == EntityType::BUILDER_BASE) 
            ||(entity.entityType == EntityType::MELEE_BASE)
            ||(entity.entityType == EntityType::RANGED_BASE)
            ||(entity.entityType == EntityType::TURRET)
            ||(entity.entityType == EntityType::RESOURCE)
            ||(entity.entityType == EntityType::WALL)
           )
           fillMapCells(mapBuilding, entity.position, 1, entitySize, 1);
        
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
            if ((entity.entityType == EntityType::RANGED_UNIT)
                ||(entity.entityType == EntityType::MELEE_UNIT))
                fillMapCells(mapEnemy, entity.position, entity.id, 1, 0);
        }
        else
        {
            playerPopulation[*entity.playerId].available += entityProperties[entity.entityType].populationProvide;
            playerPopulation[*entity.playerId].inUse += entityProperties[entity.entityType].populationUse;
            switch (entity.entityType)
            {
            case EntityType::BUILDER_UNIT:
                myBuiderUnits.emplace_back(entity);
                countBuilderUnits += 1;
                break;
            case EntityType::RANGED_UNIT:
                myAttackUnits.emplace_back(entity);
                fillMapCells(mapAlly, entity.position, entity.id, 1, 0);
                countRangeUnits += 1;
                break;
            case EntityType::MELEE_UNIT:
                myAttackUnits.emplace_back(entity);
                fillMapCells(mapAlly, entity.position, entity.id, 1, 0);
                countMeleeUnits += 1;
                break;
            case EntityType::TURRET:
                if (entity.health < entityProperties[entity.entityType].maxHealth)
                    myDamagedBuildings.emplace_back(entity);
                if (entity.active == true)
                {
                    myTurrets.emplace_back(entity);
                    fillMapCells(mapAlly, entity.position, entity.id, 1, 0);
                }
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

    // for (Entity res : resourses)
    // {
    //     if (isAvailable(mapOccupied, res.position, entityProperties[EntityType::RESOURCE].size))
    //         availableResourses.emplace_back(res);
    // }
    //
    // Fill map of potential damage from enemy units.
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
    
    // Fill map of potential attack from ally units.
    // for (Entity ent: myAttackUnits)
    // {
    //     int radius = (*entityProperties[ent.entityType].attack).attackRange;
    //     int damage = (*entityProperties[ent.entityType].attack).damage;
    //     fillDamageMap(mapAttack, ent.position, radius, damage);
    // }
    //
    // for (Entity ent: myTurrets) // TODO: Исправить расчет поля атаки турели с учетом ее размера!
    // {
    //     int radius = (*entityProperties[ent.entityType].attack).attackRange;
    //     int damage = (*entityProperties[ent.entityType].attack).damage;
    //     fillDamageMap(mapAttack, ent.position, radius, damage);
    // }

    possibleBuildPositions = findFreePosOnBuildCellMap(mapBuilding, entityProperties[EntityType::HOUSE].size);
    myAvailableResources = playersInfo[playerView.myId].resource;
    d_Resources = myAvailableResources - lastAvailableResources;
    lastAvailableResources = myAvailableResources;
    myAvailablePopulation = playerPopulation[playerView.myId].available - playerPopulation[playerView.myId].inUse;

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
        EntityAction action = chooseAttackUnitAction(entity, mapOccupied, mapAlly, mapEnemy, mapDamage);
        orders[entity.id] = action;
    }

    for (Entity entity : myTurrets)
    {
        EntityAction action;
        AttackAction attack;
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
        attack.autoAttack = std::make_shared<AutoAttack>(autoAttack);
        action.attackAction = std::make_shared<AttackAction>(attack);
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

    // float tickBalance = unitBalance(playerView.currentTick);
    // float currentBalance = float(myBuiderUnits.size())/float(myAttackUnits.size() + myBuiderUnits.size());

    // Color color;
    // if (myAvailableResources < 20)
    //     color = colors::red;
    // else
    //     color = colors::green;
    // this->debugData.emplace_back(new DebugData::PlacedText(ColoredVertex(debug::info_pos, Vec2Float(0, 0), colors::white), 
    //                                                        "Builders: "+to_string(countBuilderUnits), 
    //                                                                      0, 20));
    // this->debugData.emplace_back(new DebugData::PlacedText(ColoredVertex(debug::info_pos, Vec2Float(0, -20), colors::white), 
    //                                                        "Melee: "+to_string(countMeleeUnits), 
    //                                                                      0, 20));
    // this->debugData.emplace_back(new DebugData::PlacedText(ColoredVertex(debug::info_pos, Vec2Float(0, -40), colors::white), 
    //                                                        "Range: "+to_string(countRangeUnits), 
    //                                                                      0, 20));
    // this->debugData.emplace_back(new DebugData::PlacedText(ColoredVertex(std::make_shared<Vec2Float>(remontPoint.position.x, remontPoint.position.y), 
    //                                                                                                  Vec2Float(0, -40), colors::red), 
    //                                                        "X", 0, 20));
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
    int half = playerView.mapSize / 2;
    defaultTarget = Entity{-1, nullptr, EntityType::MELEE_UNIT, Vec2Int{half, half}, 0, true};
    remontPoint.entityType = EntityType::MELEE_UNIT;
    remontPoint.playerId = std::make_shared<int>(playerView.myId);
    for (Entity entity : playerView.entities)
    {
        if (entity.entityType == EntityType::RESOURCE) continue;
        if (*entity.playerId != playerView.myId) continue;

        if ((entity.position.x < half) && (entity.position.y < half))
        {
            remontPoint.position.x = playerView.mapSize / 4;
            remontPoint.position.y = playerView.mapSize / 4;
            break;
        }
        else if ((entity.position.x > half) && (entity.position.y < half))
        {
            remontPoint.position.x = playerView.mapSize - playerView.mapSize / 4;
            remontPoint.position.y = playerView.mapSize / 4;
            break;
        }
        else if ((entity.position.x < half) && (entity.position.y > half))
        {
            remontPoint.position.x = playerView.mapSize / 4;
            remontPoint.position.y = playerView.mapSize - playerView.mapSize / 4;
            break;
        }
        else
        {
            remontPoint.position.x = playerView.mapSize - playerView.mapSize / 4;
            remontPoint.position.y = playerView.mapSize - playerView.mapSize / 4;
            break;
        } 
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

        // ColoredVertex A{std::make_shared<Vec2Float>(entity.position.x, entity.position.y), {0, 0}, colors::red};
        // ColoredVertex B{std::make_shared<Vec2Float>(nearestDamagedBuilding.position.x, nearestDamagedBuilding.position.y), {0, 0}, colors::green};
        // std::vector<ColoredVertex> line{A, B};
        // this->debugData.emplace_back(new DebugData::Primitives(line, PrimitiveType::LINES));

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

        // ColoredVertex A{std::make_shared<Vec2Float>(entity.position.x, entity.position.y), {0, 0}, colors::red};
        // ColoredVertex B{std::make_shared<Vec2Float>(nearestResource.position.x, nearestResource.position.y), {0, 0}, colors::red};
        // std::vector<ColoredVertex> line{A, B};
        // this->debugData.emplace_back(new DebugData::Primitives(line, PrimitiveType::LINES));

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

        // ColoredVertex A{std::make_shared<Vec2Float>(entity.position.x, entity.position.y), {0, 0}, colors::red};
        // ColoredVertex B{std::make_shared<Vec2Float>(nearestEnemy.position.x, nearestEnemy.position.y), {0, 0}, colors::red};
        // std::vector<ColoredVertex> line{A, B};
        // this->debugData.emplace_back(new DebugData::Primitives(line, PrimitiveType::LINES));
    }
    return resultAction;
}

EntityAction MyStrategy::chooseRecruitUnitAction(Entity& entity, const PlayerView& playerView)
{
    EntityAction resultAction;
    float tickBalance = unitBalance(playerView.currentTick);
    float currentBalance = float(myBuiderUnits.size() + builderUnitOrder)/float(myAttackUnits.size() + atackUnitOrder + myBuiderUnits.size() + builderUnitOrder);
    bool reqruitBuilder = false;
    if (myAvailableResources < 20)
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
    else if (entity.entityType == EntityType::MELEE_BASE)
    {
        if ((currentBalance >= tickBalance) && playerView.currentTick <= 400)
        {
            BuildAction action;
            action.entityType = EntityType::MELEE_UNIT;
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

EntityAction MyStrategy::chooseAttackUnitAction(Entity& entity, 
                                                vector<vector<int>>& mapOccupied, 
                                                vector<vector<int>>& mapAlly, 
                                                vector<vector<int>>& mapEnemy,
                                                vector<vector<int>>& mapDamage)
{
    EntityAction resultAction;
    resultAction.attackAction = nullptr;
    resultAction.moveAction = nullptr;
    resultAction.buildAction = nullptr;
    resultAction.repairAction = nullptr;

    bool ignoreAvailable = false;
    if (entity.entityType == EntityType::RANGED_UNIT || entity.entityType == EntityType::TURRET)
        ignoreAvailable = true;
    
    Entity nearestEnemy = findNearestEntity(entity, enemyEntities, mapOccupied, ignoreAvailable);

    int myAttackRange = (*entityProperties[entity.entityType].attack).attackRange;

    int nearbyEnemys = countUnitsInRadius(entity.position, 7, mapEnemy);
    int nearbyAllies = countUnitsInRadius(entity.position, 3, mapAlly);

    if (nearestEnemy.id != -1) // Враг обнаружен.
    {
        if (distance(entity, nearestEnemy) >= myAttackRange + 2) // Враг далеко, идем к нему.
        {
            MoveAction action;
            action.target = nearestEnemy.position;
            action.breakThrough = true;
            action.findClosestPosition = true;
            resultAction.moveAction = std::make_shared<MoveAction>(action);

            // ColoredVertex A{std::make_shared<Vec2Float>(entity.position.x, entity.position.y), {0, 0}, colors::green};
            // ColoredVertex B{std::make_shared<Vec2Float>(nearestEnemy.position.x, nearestEnemy.position.y), {0, 0}, colors::green};
            // std::vector<ColoredVertex> line{A, B};
            // this->debugData.emplace_back(new DebugData::Primitives(line, PrimitiveType::LINES));
        }
        else if (distance(entity, nearestEnemy) >= myAttackRange) // Враг в буферной зоне. Принимаем решение наступать, ждать врага или отступить.
        {
            if (nearbyAllies > nearbyEnemys) // У нас перевес - атакуем.
            {
                MoveAction action;
                action.target = nearestEnemy.position;
                action.breakThrough = true;
                action.findClosestPosition = true;
                resultAction.moveAction = std::make_shared<MoveAction>(action);

                // ColoredVertex A{std::make_shared<Vec2Float>(entity.position.x, entity.position.y), {0, 0}, colors::red};
                // ColoredVertex B{std::make_shared<Vec2Float>(nearestEnemy.position.x, nearestEnemy.position.y), {0, 0}, colors::red};
                // std::vector<ColoredVertex> line{A, B};
                // this->debugData.emplace_back(new DebugData::Primitives(line, PrimitiveType::LINES));
            }
            else if (nearbyAllies == nearbyEnemys) // Паритет
            {
                if (entity.entityType == EntityType::RANGED_UNIT) // Если наш юнит РЕНДЖ, то ждем
                {
                    MoveAction action;
                    action.target = entity.position;
                    action.breakThrough = false;
                    action.findClosestPosition = true;
                    resultAction.moveAction = std::make_shared<MoveAction>(action);
                }
                else // Наш юнит МИЛИ
                {
                    if (nearestEnemy.entityType == EntityType::MELEE_UNIT) // Если враг тоже МИЛИ, то ждем.
                    {
                        MoveAction action;
                        action.target = entity.position;
                        action.breakThrough = false;
                        action.findClosestPosition = true;
                        resultAction.moveAction = std::make_shared<MoveAction>(action);
                    }
                    else // Атакуем.
                    {
                        AttackAction action;
                        action.target = std::make_shared<int>(nearestEnemy.id);
                        resultAction.attackAction = std::make_shared<AttackAction>(action);
                    }
                }

                // ColoredVertex A{std::make_shared<Vec2Float>(entity.position.x, entity.position.y), {0, 0}, colors::white};
                // ColoredVertex B{std::make_shared<Vec2Float>(nearestEnemy.position.x, nearestEnemy.position.y), {0, 0}, colors::white};
                // std::vector<ColoredVertex> line{A, B};
                // this->debugData.emplace_back(new DebugData::Primitives(line, PrimitiveType::LINES));
            }
            else // Врагов больше - отступаем, если возможно, если не возможно, то ждем врага.
            {
                MoveAction action;
                Vec2Int retreatPos = getRetreatPos(entity.position, mapDamage, mapOccupied);
                action.target = retreatPos;
                action.breakThrough = false;
                action.findClosestPosition = true;
                resultAction.moveAction = std::make_shared<MoveAction>(action);

                // ColoredVertex A{std::make_shared<Vec2Float>(entity.position.x, entity.position.y), {0, 0}, colors::white};
                // ColoredVertex B{std::make_shared<Vec2Float>(nearestEnemy.position.x, nearestEnemy.position.y), {0, 0}, colors::white};
                // std::vector<ColoredVertex> line{A, B};
                // this->debugData.emplace_back(new DebugData::Primitives(line, PrimitiveType::LINES));
            }
        }
        else // Враг в радиусе поражения. Принимаем решение атаковать врага или отступать.
        {
            if ((nearestEnemy.entityType == EntityType::RANGED_UNIT) // Если враг РЕНДЖ - атакуем.
                ||(nearestEnemy.entityType == EntityType::TURRET))
            {
                AttackAction action;
                action.target = std::make_shared<int>(nearestEnemy.id);
                resultAction.attackAction = std::make_shared<AttackAction>(action);
            }
            else if (nearestEnemy.entityType == EntityType::MELEE_UNIT) // Если враг МИЛИ
            {
                if (entity.entityType == EntityType::MELEE_UNIT) // Если мой юнит МИЛИ, то атакуем без вариантов.
                {
                    AttackAction action;
                    action.target = std::make_shared<int>(nearestEnemy.id);
                    resultAction.attackAction = std::make_shared<AttackAction>(action);
                }
                else // Мой юнит РЕНДЖ
                {
                    if (distance(entity, nearestEnemy) > 1) // Враг еще не подошел на дистанцию атаки. Можно атаковать.
                    {
                        AttackAction action;
                        action.target = std::make_shared<int>(nearestEnemy.id);
                        resultAction.attackAction = std::make_shared<AttackAction>(action);   
                    }
                    else // Враг опасно близко. Надо попытаться отступить.
                    {
                        Vec2Int retreatPos = getRetreatPos(entity.position, mapDamage, mapOccupied);

                        if ((retreatPos.x == entity.position.x) && (retreatPos.y == entity.position.y)) // Если стоять на месте - лучший вариант, то атакуем.
                        {
                            AttackAction action;
                            action.target = std::make_shared<int>(nearestEnemy.id);
                            resultAction.attackAction = std::make_shared<AttackAction>(action);   
                        }
                        else
                        {
                            MoveAction action;
                            action.target = retreatPos;
                            action.breakThrough = false;
                            action.findClosestPosition = true;
                            resultAction.moveAction = std::make_shared<MoveAction>(action);
                        }
                    }
                }
            }
            else // Если враг не РЕНДЖ и не МИЛИ. То есть раб или здание - атакуем.
            {
                AttackAction action;
                action.target = std::make_shared<int>(nearestEnemy.id);
                resultAction.attackAction = std::make_shared<AttackAction>(action);
            }
        }
    }
    else // Враг не обнаружен.
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
        if ((d < min_dist) && (ignoreAvailable || d == 0 || isAvailable(map, e.position, entityProperties[e.entityType].size)))
        {
            min_dist = d;
            nearestEntity = e;
        }
    }
    return nearestEntity;
}

// Entity MyStrategy::findNearestReachableResource(Entity& entity, std::unordered_map<int, Entity>& entities)
// {
//     int min_dist = 100000;
//     int nearestEntityId = -1;
//     Entity nearestEntity;
//     nearestEntity.id = -1;
//     for (auto& e : entities)
//     {
//         int d = distance(entity, e.second);
//         if (d < min_dist)
//         {
//             min_dist = d;
//             nearestEntity = e.second;
//             nearestEntityId = e.first;
//         }
//     }
//     entities.erase(nearestEntityId);
//     return nearestEntity;
// }
//
// EntityAction MyStrategy::getAttackUnitAction(Entity& entity, 
//                                              std::vector<std::vector<int>>& mapAttack, 
//                                              std::vector<std::vector<int>>& mapDamage,
//                                              std::vector<std::vector<int>>& mapOccupied)
// {
//     EntityAction resultAction;
//     resultAction.attackAction = nullptr;
//     resultAction.moveAction = nullptr;
//     resultAction.repairAction = nullptr;
//     resultAction.buildAction = nullptr;
//
//     Entity target;
//     Entity nearestEnemy = findNearestEntity(entity, enemyEntities, mapOccupied, true);
//     if (nearestEnemy.id == -1)
//         target = defaultTarget;
//     else
//         target = nearestEnemy;
//   
//     MoveAnalysis step_top = analyzeMove(entity, 0, 1, target, mapAttack, mapDamage, mapOccupied);
//     MoveAnalysis step_right = analyzeMove(entity, 1, 0, target, mapAttack, mapDamage, mapOccupied);
//     MoveAnalysis step_bot = analyzeMove(entity, 0, -1, target, mapAttack, mapDamage, mapOccupied);
//     MoveAnalysis step_left = analyzeMove(entity, -1, 0, target, mapAttack, mapDamage, mapOccupied);
//     MoveAnalysis stay_here = analyzeMove(entity, 0, 0, target, mapAttack, mapDamage, mapOccupied);
//
//     float best_total = stay_here.total;
//     int best_step = 0;
//     float needAttack = false;
//     if ((step_top.total < best_total)&&(step_top.canGo != 0))
//     {
//         best_total = step_top.total;
//         best_step = 1;
//         if (step_top.canGo == 2)
//             needAttack = true;
//     }
//     if ((step_right.total < best_total)&&(step_right.canGo != 0))
//     {
//         best_total = step_right.total;
//         best_step = 2;
//         if (step_right.canGo == 2)
//             needAttack = true;
//     }
//     if ((step_bot.total < best_total)&&(step_bot.canGo != 0))
//     {
//         best_total = step_bot.total;
//         best_step = 3;
//         if (step_bot.canGo == 2)
//             needAttack = true;
//     }
//     if ((step_left.total < best_total)&&(step_left.canGo != 0))
//     {
//         best_total = step_left.total;
//         best_step = 4;
//         if (step_left.canGo == 2)
//             needAttack = true;
//     }
//
//     MoveAction action;
//     switch (best_step)
//     {
//     case 0:
//         action.target = Vec2Int{entity.position.x, entity.position.y};
//         action.breakThrough = true;
//         action.findClosestPosition = true;
//         break;
//     case 1:
//         action.target = Vec2Int{entity.position.x, entity.position.y + 1};
//         action.breakThrough = true;
//         action.findClosestPosition = true;
//         break;
//     case 2:
//         action.target = Vec2Int{entity.position.x + 1, entity.position.y};
//         action.breakThrough = true;
//         action.findClosestPosition = true;
//         break;
//     case 3:
//         action.target = Vec2Int{entity.position.x, entity.position.y - 1};
//         action.breakThrough = true;
//         action.findClosestPosition = true;
//         break;
//     case 4:
//         action.target = Vec2Int{entity.position.x - 1, entity.position.y};
//         action.breakThrough = true;
//         action.findClosestPosition = true;
//         break;
//     }
//     resultAction.moveAction = std::make_shared<MoveAction>(action);
//
//     if (nearestEnemy.id != -1)
//     {
//         int distToEnemy = distance(entity, nearestEnemy);
//         if ((distToEnemy < (*entityProperties[entity.entityType].attack).attackRange)
//             &&(entity.health - mapDamage[entity.position.x][entity.position.y] >= nearestEnemy.health - mapAttack[nearestEnemy.position.x][nearestEnemy.position.y]))
//         {
//             AttackAction attack;
//             attack.target = std::make_shared<int>(nearestEnemy.id);
//             resultAction.attackAction = std::make_shared<AttackAction>(attack);
//         }
//     }
//
//     return resultAction;
// }

// MoveAnalysis MyStrategy::analyzeMove(Entity& entity, int dx, int dy, Entity& target,
//                                      std::vector<std::vector<int>>& mapAttack, 
//                                      std::vector<std::vector<int>>& mapDamage,
//                                      std::vector<std::vector<int>>& mapOccupied)
// {
//     MoveAnalysis resultAnalysis;
// 
//     Entity e{entity};
//     int mapSize = mapAttack[0].size();
//     int step = 0;
//     int maxSteps = 5;
//     float initialDistToTarget = euclideanDist(entity, target);
//     float minDistToTarget = initialDistToTarget;
//     Entity nearestAlly = findNearestEntity(entity, myAttackUnits, mapAttack, true);
//     float initialDistToAlly = 0;
//     if (nearestAlly.id != -1)
//         initialDistToAlly = euclideanDist(entity, nearestAlly);
//     float minDistToAlly = initialDistToAlly;
// 
//     resultAnalysis.canGo = 0;
//     resultAnalysis.distanceToTarget = 0;
//     resultAnalysis.distanceToAlly = 0;
//     resultAnalysis.maxDamage = mapDamage[entity.position.x][entity.position.y];
// 
//     if ((dx == 0) && (dy == 0))
//     {
//         resultAnalysis.canGo = 1;
//     }
//     else
//     {
//         while (step < maxSteps)
//         {
//             if ((e.position.x + dx < 0) || (e.position.x + dx >= mapSize))
//                 break;
//             if ((e.position.y + dy < 0) || (e.position.y + dy >= mapSize))
//                 break;
//             e.position.x += dx;
//             e.position.y += dy;
//             if (mapOccupied[e.position.x][e.position.y] >= 2)
//             {
//                 if (step == 0)
//                     resultAnalysis.canGo = 0;
//                 break;
//             }
//             if ((mapOccupied[e.position.x][e.position.y] == 1) && (step == 0))
//                 resultAnalysis.canGo = 2;
//             else
//                 resultAnalysis.canGo = 1;
// 
//             int damage = mapDamage[e.position.x][e.position.y];
//             if (damage > resultAnalysis.maxDamage)
//                 resultAnalysis.maxDamage = damage;
// 
//             float dist = euclideanDist(e, target);
//             if (dist < minDistToTarget)
//                 minDistToTarget = dist;
// 
//             nearestAlly = findNearestEntity(e, myAttackUnits, mapAttack, true);
//             if (nearestAlly.id != -1)
//             {
//                 float dist = euclideanDist(e, nearestAlly);
//                 if (dist < minDistToAlly)
//                     minDistToAlly = dist;
//             }
//             step++;
//         }
//     }
//     resultAnalysis.distanceToTarget = minDistToTarget - initialDistToTarget;
//     resultAnalysis.distanceToAlly = minDistToAlly - initialDistToAlly;
//     resultAnalysis.total = resultAnalysis.maxDamage + resultAnalysis.distanceToTarget + resultAnalysis.distanceToAlly;
//     return resultAnalysis;
// }

// float MyStrategy::euclideanDist(Entity& e1, Entity& e2)
// {
//     float x2 = (e1.position.x - e2.position.x) * (e1.position.x - e2.position.x);
//     float y2 = (e1.position.y - e2.position.y) * (e1.position.y - e2.position.y);
//     return sqrt(x2 + y2);
// }
