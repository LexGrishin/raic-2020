#include "MyStrategy.hpp"
#include <exception>
#include <iostream>
#include <string>
#include <typeinfo>
#include <cmath>
#include <algorithm>
#include <queue>

using std::cout;
using std::endl;
using std::to_string;

MyStrategy::MyStrategy() {}

Action MyStrategy::getAction(const PlayerView playerView, DebugInterface* debugInterface)
{
    debugInterface->send(DebugCommand::SetAutoFlush(false));
    this->debugData.clear();
    std::unordered_map<int, EntityAction> orders{};

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
    
    std::vector<std::vector<int>> mapOccupied(playerView.mapSize, std::vector<int>(playerView.mapSize, -1)); // Карта для поиска пути.
    std::vector<std::vector<int>> mapBuilding(playerView.mapSize, std::vector<int>(playerView.mapSize, 0)); // Только постройки и ресурсы. 1 - клетка занята, 0 - клетка свободна. Заполняется с паддингом 1.
    std::vector<std::vector<int>> mapAttack(playerView.mapSize, std::vector<int>(playerView.mapSize, 0)); // Суммарный дамаг по клетке.
    std::vector<std::vector<int>> mapDamage(playerView.mapSize, std::vector<int>(playerView.mapSize, 0));
    std::vector<std::vector<Vec2Int>> mapCameFrom(playerView.mapSize, std::vector<Vec2Int>(playerView.mapSize, Vec2Int{-1, -1}));

    resourses.clear(); 
    enemyEntities.clear();
    myUnits.clear();
    myTurrets.clear();
    myBuildings.clear();
    myDamagedBuildings.clear();

    countBuilderUnits = 0;
    countMeleeUnits = 0;
    countRangeUnits = 0;

    // Fill maps and unit arrays.
    for (Entity entity : playerView.entities)
    {
        if ((entity.entityType == EntityType::HOUSE)
            ||(entity.entityType == EntityType::BUILDER_BASE) 
            ||(entity.entityType == EntityType::MELEE_BASE)
            ||(entity.entityType == EntityType::RANGED_BASE)
            ||(entity.entityType == EntityType::TURRET)
            ||(entity.entityType == EntityType::RESOURCE)
            ||(entity.entityType == EntityType::WALL)
           )
           fillMapCells(mapBuilding, entity.position, 1, entity.getSize(), 1);
        
        if (entity.playerId == nullptr)
        {
            resourses.emplace_back(entity);
            fillMapCells(mapOccupied, entity.position, entity.entityType, entity.getSize(), 0);
        }
        else if (*entity.playerId != playerView.myId)
        {
            if (entity.active)
            {
                playerPopulation[*entity.playerId].available += entity.populationProvide();
                playerPopulation[*entity.playerId].inUse += entity.populationUse();
            }
            enemyEntities.emplace_back(entity);
            fillMapCells(mapOccupied, entity.position, entity.entityType, entity.getSize(), 0);
        }
        else
        {
            if (entity.active)
            {
                playerPopulation[*entity.playerId].available += entity.populationProvide();
                playerPopulation[*entity.playerId].inUse += entity.populationUse();
            }
            
            switch (entity.entityType)
            {
            case EntityType::BUILDER_UNIT:
                myUnits.emplace_back(entity);
                countBuilderUnits += 1;
                break;
            case EntityType::RANGED_UNIT:
                myUnits.emplace_back(entity);
                countRangeUnits += 1;
                break;
            case EntityType::MELEE_UNIT:
                myUnits.emplace_back(entity);
                countMeleeUnits += 1;
                break;
            case EntityType::TURRET:
                if (entity.health < entity.maxHealth()) myDamagedBuildings.emplace_back(entity);
                if (entity.active == true) myTurrets.emplace_back(entity);
                fillMapCells(mapOccupied, entity.position, entity.entityType, entity.getSize(), 0);
                break;
            case EntityType::BUILDER_BASE:
                if (entity.health < entity.maxHealth()) myDamagedBuildings.emplace_back(entity);
                if (entity.active == true) myBuildings.emplace_back(entity);
                fillMapCells(mapOccupied, entity.position, entity.entityType, entity.getSize(), 0);
                countBase += 1;
                break;
            case EntityType::MELEE_BASE:
                if (entity.health < entity.maxHealth()) myDamagedBuildings.emplace_back(entity);
                if (entity.active == true) myBuildings.emplace_back(entity);
                fillMapCells(mapOccupied, entity.position, entity.entityType, entity.getSize(), 0);
                countMeleBase += 1;
                break;
            case EntityType::RANGED_BASE:
                if (entity.health < entity.maxHealth()) myDamagedBuildings.emplace_back(entity);
                if (entity.active == true) myBuildings.emplace_back(entity);
                fillMapCells(mapOccupied, entity.position, entity.entityType, entity.getSize(), 0);
                countRangeBase += 1;
                break;
            case EntityType::HOUSE:
                if (entity.health < entity.maxHealth()) myDamagedBuildings.emplace_back(entity);
                fillMapCells(mapOccupied, entity.position, entity.entityType, entity.getSize(), 0);
                break;
            case EntityType::WALL:
                if (entity.health < entity.maxHealth()) myDamagedBuildings.emplace_back(entity);
                fillMapCells(mapOccupied, entity.position, entity.entityType, entity.getSize(), 0);
                break;
            default:
                break;
            }
        }
    }
    // Fill map of potential damage from enemy units.
    for (Entity ent: enemyEntities)
    {
        if ((ent.entityType == EntityType::RANGED_UNIT)
            || (ent.entityType == EntityType::MELEE_UNIT)
            || (ent.entityType == EntityType::TURRET)
            || (ent.entityType == EntityType::BUILDER_UNIT))
        {
            fillDamageMap(mapDamage, ent.position, ent.attackRange(), ent.damage());  // TODO: Для турели атака заполняется неправильно!
        }
    }
    // Fill map of potential attack from ally units.
    for (Entity ent: myUnits)
    {
        if (ent.entityType == EntityType::RANGED_UNIT 
           || ent.entityType == EntityType::MELEE_UNIT)
        {
            fillDamageMap(mapAttack, ent.position, ent.attackRange(), ent.damage()); 
        }
    }
    for (Entity ent: myTurrets) // TODO: Исправить расчет поля атаки турели с учетом ее размера!
    {
        fillDamageMap(mapAttack, ent.position, ent.attackRange(), ent.damage());  // TODO: Для турели атака заполняется неправильно!
    }
  
    myAvailableResources = playersInfo[playerView.myId].resource;
    d_Resources = myAvailableResources - lastAvailableResources;
    lastAvailableResources = myAvailableResources;
    myAvailablePopulation = playerPopulation[playerView.myId].available - playerPopulation[playerView.myId].inUse;

    for (auto pair : buildOrder)
    {
        cout<<"builder uid: "<<pair.first<<endl;
    }
    delDeadUnitsFromBuildOrder();
    delImposibleOrders(mapBuilding);
    cout<<"BO: "<<buildOrder.size()<<endl;
    
    // 1. Фаза намерений:
    for (auto entity : myUnits)
    {
        if (entity.entityType == EntityType::BUILDER_UNIT)
        {
            getBuilderUnitIntention(entity, mapOccupied, mapDamage);
            myUnitsPriority.push(entity);
        }
        else if (entity.entityType == EntityType::RANGED_UNIT)
        {
            getRangedUnitIntention(entity, mapOccupied, mapDamage);
            myUnitsPriority.push(entity);
        }
        else
        {
            getMeleeUnitIntention(entity, mapOccupied, mapDamage);
            myUnitsPriority.push(entity);
        }
    }

    // 2. Фаза маневров:
    while (! myUnitsPriority.empty())
    {
        Entity entity = myUnitsPriority.top();
        myUnitsPriority.pop();
        EntityAction action;

        if (entity.entityType == EntityType::BUILDER_UNIT)
        {
            getBuilderUnitMove(entity, mapOccupied, mapDamage, mapCameFrom);
            action = getBuilderUnitAction(entity);
        }
        else if (entity.entityType == EntityType::RANGED_UNIT)
        {
            getRangedUnitMove(entity, mapOccupied, mapDamage, mapCameFrom);
            action = getRangedUnitAction(entity);
        }
        else
        {
            getMeleeUnitMove(entity, mapOccupied, mapDamage, mapCameFrom);
            action = getMeleeUnitAction(entity);
        }
        orders[entity.id] = action;
    }

    for (auto pair : buildOrder)
    {
        fillMapCells(mapBuilding, pair.second.position, 1, pair.second.getSize(), 1);
    }


// -----------------------------------------------------------------------------------------------------------------------------

    Entity nearestEnemyToBase = findNearestEntity(baseCenter, enemyEntities, mapOccupied, true);
    int enemyDistToBase = 100000;
    if (nearestEnemyToBase.id != -1)
        enemyDistToBase = distance(baseCenter.position, nearestEnemyToBase.position);

    for (Entity entity : myBuildings)
    {
        EntityAction action = chooseRecruitUnitAction(entity, playerView, enemyDistToBase, mapOccupied);
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

    fillBuildOrder(mapBuilding, mapOccupied, orders);

    for (int i=0; i < mapBuilding[0].size(); i++)
        for (int j=0; j < mapBuilding[0].size(); j++)
        {
            if (mapBuilding[i][j] != 0)
            {
                this->debugData.emplace_back(new DebugData::PlacedText(ColoredVertex(std::make_shared<Vec2Float>(i+0.5, j+0.5), 
                                                                         Vec2Float(0, 0), colors::red), 
                                                                         "X", 
                                                                         0.5, 10));
            }
        }

    float tickBalance = unitBalance(playerView.currentTick);
    float currentBalance = float(countBuilderUnits)/(float(countRangeUnits + countMeleeUnits + countBuilderUnits) + 0.000001);

    Color color;
    if (currentBalance < tickBalance)
        color = colors::red;
    else
        color = colors::green;
    
    this->debugData.emplace_back(new DebugData::PlacedText(ColoredVertex(debug::info_pos, 
                                                                                                     Vec2Float(0, -120), color), 
                                                           "Balance: "+to_string(currentBalance)+" / "+to_string(tickBalance), 0, 20));
    cout<<"Res: "<<resourses.size()
        <<" En U: "<<enemyEntities.size()
        <<" My U: "<<myUnits.size()
        <<" My T: "<<myTurrets.size()
        <<" My B: "<<myBuildings.size()
        <<" My D: "<<myDamagedBuildings.size()
        <<" BO: "<<buildOrder.size()
        <<endl;
    
    for (auto item : this->debugData)
    {
        debugInterface->send(DebugCommand::Add(item));
    }
    debugInterface->send(DebugCommand::Flush());
    return Action(orders);
}

void MyStrategy::debugUpdate(const PlayerView& playerView, DebugInterface& debugInterface)
{
    debugInterface.send(DebugCommand::SetAutoFlush(false));
    // debugInterface.send(DebugCommand::Clear());
    // for (auto item : this->debugData)
    // {
    //     debugInterface.send(DebugCommand::Add(item));
    // }
    // debugInterface.send(DebugCommand::Flush());
    // debugInterface.getState();
}

void MyStrategy::setGlobals(const PlayerView& playerView)
{
    this->isGlobalsSet = true;
    int half = playerView.mapSize / 2;
    remontPoint.entityType = EntityType::MELEE_UNIT;
    remontPoint.playerId = std::make_shared<int>(playerView.myId);
    baseCenter.entityType = EntityType::MELEE_UNIT;
    baseCenter.playerId = std::make_shared<int>(playerView.myId);
    enemyCenter1.entityType = EntityType::MELEE_UNIT;
    enemyCenter2.entityType = EntityType::MELEE_UNIT;
    enemyCenter2.entityType = EntityType::MELEE_UNIT;
    for (Entity entity : playerView.entities)
    {
        if (entity.entityType == EntityType::RESOURCE) continue;
        if (*entity.playerId != playerView.myId) continue;

        if ((entity.position.x < half) && (entity.position.y < half))
        {
            remontPoint.position.x = playerView.mapSize / 4;
            remontPoint.position.y = playerView.mapSize / 4;
            baseCenter.position.x = playerView.mapSize / 10;
            baseCenter.position.y = playerView.mapSize / 10;

            enemyCenter1.position.x = playerView.mapSize - playerView.mapSize / 10;
            enemyCenter1.position.y = playerView.mapSize - playerView.mapSize / 10;
            enemyCenter2.position.x = playerView.mapSize / 10;
            enemyCenter2.position.y = playerView.mapSize - playerView.mapSize / 10;
            enemyCenter3.position.x = playerView.mapSize - playerView.mapSize / 10;
            enemyCenter3.position.y = playerView.mapSize / 10;
            break;
        }
        else if ((entity.position.x > half) && (entity.position.y < half))
        {
            remontPoint.position.x = playerView.mapSize - playerView.mapSize / 4;
            remontPoint.position.y = playerView.mapSize / 4;
            baseCenter.position.x = playerView.mapSize - playerView.mapSize / 10;
            baseCenter.position.y = playerView.mapSize / 10;

            enemyCenter1.position.x = playerView.mapSize / 10;
            enemyCenter1.position.y = playerView.mapSize - playerView.mapSize / 10;
            enemyCenter2.position.x = playerView.mapSize / 10;
            enemyCenter2.position.y = playerView.mapSize / 10;
            enemyCenter3.position.x = playerView.mapSize - playerView.mapSize / 10;
            enemyCenter3.position.y = playerView.mapSize - playerView.mapSize / 10;
            break;
        }
        else if ((entity.position.x < half) && (entity.position.y > half))
        {
            remontPoint.position.x = playerView.mapSize / 4;
            remontPoint.position.y = playerView.mapSize - playerView.mapSize / 4;
            baseCenter.position.x = playerView.mapSize / 10;
            baseCenter.position.y = playerView.mapSize - playerView.mapSize / 10;

            enemyCenter1.position.x = playerView.mapSize - playerView.mapSize / 10;
            enemyCenter1.position.y = playerView.mapSize / 10;
            enemyCenter2.position.x = playerView.mapSize / 10;
            enemyCenter2.position.y = playerView.mapSize / 10;
            enemyCenter3.position.x = playerView.mapSize - playerView.mapSize / 10;
            enemyCenter3.position.y = playerView.mapSize - playerView.mapSize / 10;
            break;
        }
        else
        {
            remontPoint.position.x = playerView.mapSize - playerView.mapSize / 4;
            remontPoint.position.y = playerView.mapSize - playerView.mapSize / 4;
            baseCenter.position.x = playerView.mapSize - playerView.mapSize / 10;
            baseCenter.position.y = playerView.mapSize - playerView.mapSize / 10;

            enemyCenter1.position.x = playerView.mapSize / 10;
            enemyCenter1.position.y = playerView.mapSize / 10;
            enemyCenter2.position.x = playerView.mapSize - playerView.mapSize / 10;
            enemyCenter2.position.y = playerView.mapSize / 10;
            enemyCenter3.position.x = playerView.mapSize / 10;
            enemyCenter3.position.y = playerView.mapSize - playerView.mapSize / 10;
            break;
        } 
    }
}

EntityAction MyStrategy::chooseRecruitUnitAction(Entity& entity, const PlayerView& playerView, int enemyDistToBase, std::vector<std::vector<int>>& mapOccupied)
{
    EntityAction resultAction;
    float tickBalance = unitBalance(playerView.currentTick);
    float currentBalance = float(countBuilderUnits)/(float(countRangeUnits + countMeleeUnits + countBuilderUnits) + 0.000001);
    int halfMapSize = mapOccupied[0].size()/2;
    Vec2Int default_pos{halfMapSize, halfMapSize};
    
    if (entity.entityType == EntityType::BUILDER_BASE)
    {
        if (((currentBalance < tickBalance) || (d_Resources < 0) || playerView.currentTick < 1000) && (countBuilderUnits < 750))
        {
            Entity nearestResourse = findNearestEntity(entity, resourses, mapOccupied, true);
            if (nearestResourse.id == -1)
            {
                nearestResourse.position = default_pos;
                nearestResourse.entityType = EntityType::RESOURCE;
            }
                
            Vec2Int recruit_pos = entity.getDockingPos(nearestResourse.position, mapOccupied);
            
            if (recruit_pos != Vec2Int{-1, -1})
            {
                BuildAction action;
                action.entityType = EntityType::BUILDER_UNIT;
                action.position = recruit_pos;
                resultAction.buildAction = std::make_shared<BuildAction>(action);
            }
            else resultAction.buildAction = nullptr;
        }
        else resultAction.buildAction = nullptr;
    }
    else if (entity.entityType == EntityType::RANGED_BASE)
    {
        if ((currentBalance >= tickBalance) || myAvailableResources > 200)
        {
            Entity nearestEnemy = findNearestEntity(entity, enemyEntities, mapOccupied, true);
            if (nearestEnemy.id == -1)
            {
                nearestEnemy.position = default_pos;
                nearestEnemy.entityType = EntityType::RANGED_UNIT;
            }
            Vec2Int recruit_pos = entity.getDockingPos(nearestEnemy.position, mapOccupied);
            if (recruit_pos != Vec2Int{-1, -1})
            {
                BuildAction action;
                action.entityType = EntityType::RANGED_UNIT;
                action.position = recruit_pos;
                resultAction.buildAction = std::make_shared<BuildAction>(action);
            }
            else resultAction.buildAction = nullptr;
        }
        else resultAction.buildAction = nullptr;
    }
    else if (entity.entityType == EntityType::MELEE_BASE)
    {
        if (enemyDistToBase < 30)
        {
            Entity nearestEnemy = findNearestEntity(entity, enemyEntities, mapOccupied, true);
            if (nearestEnemy.id == -1)
            {
                nearestEnemy.position = default_pos;
                nearestEnemy.entityType = EntityType::RANGED_UNIT;
            }
            Vec2Int recruit_pos = entity.getDockingPos(nearestEnemy.position, mapOccupied);
            if (recruit_pos != Vec2Int{-1, -1})
            {
                BuildAction action;
                action.entityType = EntityType::MELEE_UNIT;
                action.position = recruit_pos;
                resultAction.buildAction = std::make_shared<BuildAction>(action);
            }
            else resultAction.buildAction = nullptr;
        }
        else resultAction.buildAction = nullptr;
    }
    else resultAction.buildAction = nullptr;  
    
    return resultAction;
}

// EntityAction MyStrategy::chooseRangeUnitAction(Entity& entity, 
//                                                 std::vector<std::vector<int>>& mapOccupied, 
//                                                 std::vector<std::vector<int>>& mapAlly, 
//                                                 std::vector<std::vector<int>>& mapEnemy,
//                                                 std::vector<std::vector<int>>& mapDamage)
// {
//     EntityAction resultAction;
//     resultAction.attackAction = nullptr;
//     resultAction.moveAction = nullptr;
//     resultAction.buildAction = nullptr;
//     resultAction.repairAction = nullptr;
    
//     Entity nearestEnemy = findNearestEntity(entity, enemyEntities);

//     int myAttackRange = (*entityProperties[entity.entityType].attack).attackRange;
//     int nearbyEnemys = countUnitsInRadius(entity.position, 7, mapEnemy);
//     int nearbyAllies = countUnitsInRadius(entity.position, 3, mapAlly);

//     if (nearestEnemy.id != -1) // Враг обнаружен.
//     {
//         if (distance(entity, nearestEnemy) >= myAttackRange + 2) // Враг далеко.
//         {
//             if (countRangeUnits + countMeleeUnits > 5) // Если союзники рядом, то идем на врага.
//             {
//                 MoveAction action;
//                 action.target = nearestEnemy.position;
//                 action.breakThrough = true;
//                 action.findClosestPosition = true;
//                 resultAction.moveAction = std::make_shared<MoveAction>(action);
//             }
//             else // Иначе, идем в точку сбора
//             {
//                 MoveAction action;
//                 action.target = remontPoint.position;
//                 action.breakThrough = false;
//                 action.findClosestPosition = true;
//                 resultAction.moveAction = std::make_shared<MoveAction>(action);
//             }
            

//             // ColoredVertex A{std::make_shared<Vec2Float>(entity.position.x, entity.position.y), {0, 0}, colors::green};
//             // ColoredVertex B{std::make_shared<Vec2Float>(nearestEnemy.position.x, nearestEnemy.position.y), {0, 0}, colors::green};
//             // std::vector<ColoredVertex> line{A, B};
//             // this->debugData.emplace_back(new DebugData::Primitives(line, PrimitiveType::LINES));
//         }
//         else if (distance(entity, nearestEnemy) >= myAttackRange) // Враг в буферной зоне. Принимаем решение наступать, ждать врага или отступить.
//         {
//             if (nearbyAllies >= nearbyEnemys) // У нас перевес - атакуем.
//             {
//                 MoveAction action;
//                 action.target = nearestEnemy.position;
//                 action.breakThrough = true;
//                 action.findClosestPosition = true;
//                 resultAction.moveAction = std::make_shared<MoveAction>(action);

//                 // ColoredVertex A{std::make_shared<Vec2Float>(entity.position.x, entity.position.y), {0, 0}, colors::red};
//                 // ColoredVertex B{std::make_shared<Vec2Float>(nearestEnemy.position.x, nearestEnemy.position.y), {0, 0}, colors::red};
//                 // std::vector<ColoredVertex> line{A, B};
//                 // this->debugData.emplace_back(new DebugData::Primitives(line, PrimitiveType::LINES));
//             }
//             else if (nearbyAllies == nearbyEnemys) // Паритет, ждем.
//             {
//                 MoveAction action;
//                 action.target = entity.position;
//                 action.breakThrough = false;
//                 action.findClosestPosition = true;
//                 resultAction.moveAction = std::make_shared<MoveAction>(action);

//                 // ColoredVertex A{std::make_shared<Vec2Float>(entity.position.x, entity.position.y), {0, 0}, colors::white};
//                 // ColoredVertex B{std::make_shared<Vec2Float>(nearestEnemy.position.x, nearestEnemy.position.y), {0, 0}, colors::white};
//                 // std::vector<ColoredVertex> line{A, B};
//                 // this->debugData.emplace_back(new DebugData::Primitives(line, PrimitiveType::LINES));
//             }
//             else // Врагов больше - отступаем, если возможно, если не возможно, то ждем врага.
//             {
//                 MoveAction action;
//                 Vec2Int retreatPos = getRetreatPos(entity.position, mapDamage, mapOccupied);
//                 action.target = retreatPos;
//                 action.breakThrough = false;
//                 action.findClosestPosition = true;
//                 resultAction.moveAction = std::make_shared<MoveAction>(action);

//                 // ColoredVertex A{std::make_shared<Vec2Float>(entity.position.x, entity.position.y), {0, 0}, colors::white};
//                 // ColoredVertex B{std::make_shared<Vec2Float>(nearestEnemy.position.x, nearestEnemy.position.y), {0, 0}, colors::white};
//                 // std::vector<ColoredVertex> line{A, B};
//                 // this->debugData.emplace_back(new DebugData::Primitives(line, PrimitiveType::LINES));
//             }
//         }
//         else // Враг в радиусе поражения. Принимаем решение атаковать врага или отступать.
//         {
//             if ((nearestEnemy.entityType == EntityType::RANGED_UNIT) // Если враг РЕНДЖ - атакуем.
//                 ||(nearestEnemy.entityType == EntityType::TURRET))
//             {
//                 AttackAction action;
//                 action.target = std::make_shared<int>(nearestEnemy.id);
//                 resultAction.attackAction = std::make_shared<AttackAction>(action);
//             }
//             else if (nearestEnemy.entityType == EntityType::MELEE_UNIT) // Если враг МИЛИ
//             {
//                 if (distance(entity, nearestEnemy) > 1) // Враг еще не подошел на дистанцию атаки. Можно атаковать.
//                 {
//                     AttackAction action;
//                     action.target = std::make_shared<int>(nearestEnemy.id);
//                     resultAction.attackAction = std::make_shared<AttackAction>(action);   
//                 }
//                 else // Враг опасно близко. Надо попытаться отступить.
//                 {
//                     Vec2Int retreatPos = getRetreatPos(entity.position, mapDamage, mapOccupied);

//                     if ((retreatPos.x == entity.position.x) && (retreatPos.y == entity.position.y)) // Если стоять на месте - лучший вариант, то атакуем.
//                     {
//                         AttackAction action;
//                         action.target = std::make_shared<int>(nearestEnemy.id);
//                         resultAction.attackAction = std::make_shared<AttackAction>(action);   
//                     }
//                     else
//                     {
//                         MoveAction action;
//                         action.target = retreatPos;
//                         action.breakThrough = false;
//                         action.findClosestPosition = true;
//                         resultAction.moveAction = std::make_shared<MoveAction>(action);
//                     }
//                 }
//             }
//             else // Если враг не РЕНДЖ и не МИЛИ. То есть раб или здание - атакуем.
//             {
//                 AttackAction action;
//                 action.target = std::make_shared<int>(nearestEnemy.id);
//                 resultAction.attackAction = std::make_shared<AttackAction>(action);
//             }
//         }
//     }
//     else // Враг не обнаружен.
//     {
//         if (entity.id % 3 == 1)
//         {
//             MoveAction action;
//             action.target = enemyCenter1.position;
//             action.breakThrough = true;
//             action.findClosestPosition = true;
//             resultAction.moveAction = std::make_shared<MoveAction>(action);
//         }
//         else if (entity.id % 3 == 2)
//         {
//             MoveAction action;
//             action.target = enemyCenter2.position;
//             action.breakThrough = true;
//             action.findClosestPosition = true;
//             resultAction.moveAction = std::make_shared<MoveAction>(action);
//         }
//         else
//         {
//             MoveAction action;
//             action.target = enemyCenter3.position;
//             action.breakThrough = true;
//             action.findClosestPosition = true;
//             resultAction.moveAction = std::make_shared<MoveAction>(action);
//         }
//     }
//     return resultAction;
// }

// EntityAction MyStrategy::chooseMeleeUnitAction(Entity& entity, 
//                                                 std::vector<std::vector<int>>& mapOccupied, 
//                                                 std::vector<std::vector<int>>& mapAlly, 
//                                                 std::vector<std::vector<int>>& mapEnemy,
//                                                 std::vector<std::vector<int>>& mapDamage)
// {
//     EntityAction resultAction;
//     resultAction.attackAction = nullptr;
//     resultAction.moveAction = nullptr;
//     resultAction.buildAction = nullptr;
//     resultAction.repairAction = nullptr;
    
//     Entity nearestEnemy = findNearestEntity(entity, enemyEntities);

//     if (nearestEnemy.id != -1) // Враг обнаружен.
//     {
//             MoveAction action;
//             action.target = nearestEnemy.position;
//             action.breakThrough = true;
//             action.findClosestPosition = true;
//             resultAction.moveAction = std::make_shared<MoveAction>(action); 
//             ColoredVertex A{std::make_shared<Vec2Float>(entity.position.x, entity.position.y), {0, 0}, colors::red};
//             ColoredVertex B{std::make_shared<Vec2Float>(nearestEnemy.position.x, nearestEnemy.position.y), {0, 0}, colors::red};
//             std::vector<ColoredVertex> line{A, B};
//             this->debugData.emplace_back(new DebugData::Primitives(line, PrimitiveType::LINES));
//     }
//     else // Враг не обнаружен.
//     {
//         AttackAction action;
//         AutoAttack autoAttack;
//         autoAttack.pathfindRange = 160;
//         autoAttack.validTargets = {EntityType::RANGED_UNIT,
//                                 EntityType::MELEE_UNIT,
//                                 EntityType::TURRET,
//                                 EntityType::BUILDER_UNIT,
//                                 EntityType::RANGED_BASE,
//                                 EntityType::MELEE_BASE,
//                                 EntityType::BUILDER_BASE,
//                                 EntityType::HOUSE
//                                 };
//         action.autoAttack = std::make_shared<AutoAttack>(autoAttack);
//         resultAction.attackAction = std::make_shared<AttackAction>(action);
//     }
//     return resultAction;
// }

// -------------------------------------------------------------------------------------------------------------------------------------------

Entity MyStrategy::findNearestEntity(Entity& entity, std::vector<Entity>& entities, std::vector<std::vector<int>>& mapOccupied, bool direct)
{
    int min_dist = 100000;
    Entity nearestEntity;
    nearestEntity.id = -1;
    nearestEntity.position = Vec2Int{-1, -1};
    
    if (direct)
    {
        for (auto ent : entities)
        {
            auto dist = distance(ent.position, entity.position);
            if (dist < min_dist)
            {
                min_dist = dist;
                nearestEntity = ent;
            }
        }
    }
    else
    {
        for (auto ent : entities)
        {
            auto dockingPos = ent.getDockingPos(entity.position, mapOccupied);
            if (dockingPos != Vec2Int{-1, -1})
            {
                auto dockingDist = distance(dockingPos, entity.position);
                if (dockingDist < min_dist)
                {
                    min_dist = dockingDist;
                    nearestEntity = ent;
                }
            }
        }
    }
    return nearestEntity;
}

void MyStrategy::getBuilderUnitIntention(Entity& entity, 
                                         std::vector<std::vector<int>>& mapOccupied,
                                         std::vector<std::vector<int>>& mapDamage)
{
    Entity nearestResource = findNearestEntity(entity, resourses, mapOccupied, false);
    Vec2Int nearestResDockingPos{-1, -1};
    int distToRes = 100000;
    if (nearestResource.id != -1)
    {
        nearestResDockingPos = nearestResource.getDockingPos(entity.position, mapOccupied);
        distToRes = distance(entity.position, nearestResDockingPos);
    }

    Entity nearestDamagedBuilding = findNearestEntity(entity, myDamagedBuildings, mapOccupied, false);
    Vec2Int nearestDamagedBuildingDockingPos{-1, -1};
    int distToDamaged = 100000;
    if (nearestDamagedBuilding.id != -1)
    {
        nearestDamagedBuildingDockingPos = nearestDamagedBuilding.getDockingPos(entity.position, mapOccupied);
        distToDamaged = distance(entity.position, nearestDamagedBuildingDockingPos);
    }

    bool isBusy = false;
    Entity buildEntity;
    Vec2Int nearestBuildDockingPos{-1, -1};
    int distToBuild = 100000;
    if (buildOrder.find(entity.id) != buildOrder.end()) 
    {
        cout<<"Busy unit found: "<<entity.id<<endl;
        isBusy = true;
        buildEntity = buildOrder[entity.id];
        nearestBuildDockingPos = buildEntity.getDockingPos(entity.position, mapOccupied);
        distToBuild = distance(entity.position, nearestBuildDockingPos);
    }
    bool needRetreat = false;    
    int radius = 2;
    int stayDamage = countDamageSum(entity.position, radius, mapDamage);
    if (stayDamage > 4) needRetreat = true;

    if (needRetreat)
    {
        Vec2Int retreatPos = getRetreatPos(entity.position, mapDamage, mapOccupied);
        entity.nextStep = retreatPos;
        entity.target = Vec2Int{-1, -1};
        entity.priority = 1;
    }
    else if (isBusy)
    {
        cout<<"Busy unit: "<<entity.id<<" Build pos:"<<nearestBuildDockingPos.x<<", "<<nearestBuildDockingPos.y<<endl;
        WayPoint point = entity.astar(nearestBuildDockingPos, mapOccupied);
        cout<<"Busy unit: "<<entity.id<<" target Build pos:"<<entity.target.x<<", "<<entity.target.y<<endl;
        entity.priority = distToBuild;
        entity.buildTarget = std::make_shared<Entity>(buildOrder[entity.id]);
    }
    else if ((nearestDamagedBuilding.id != -1) && (distToDamaged < 3 || (distToDamaged < 15 && (entity.id % 8 == 0))))
    {
        WayPoint point = entity.astar(nearestDamagedBuildingDockingPos, mapOccupied);
        entity.priority = distToDamaged;
        entity.repairTarget = std::make_shared<Entity>(nearestDamagedBuilding);
    }
    else if (nearestResource.id != -1)
    {
        WayPoint point = entity.astar(nearestResDockingPos, mapOccupied);
        entity.priority = distToRes;
        entity.attackTarget = std::make_shared<Entity>(nearestResource);
    }
    else
    {
        if (entity.id % 3 == 0)
        {
            WayPoint point = entity.astar(enemyCenter1.position, mapOccupied);
            entity.priority = distance(entity.position, enemyCenter1.position);
        }
        else if (entity.id % 3 == 1)
        {
            WayPoint point = entity.astar(enemyCenter2.position, mapOccupied);
            entity.priority = distance(entity.position, enemyCenter2.position);
        }
        else
        {
            WayPoint point = entity.astar(enemyCenter3.position, mapOccupied);
            entity.priority = distance(entity.position, enemyCenter3.position);
        }  
    }

    // ColoredVertex A{std::make_shared<Vec2Float>(entity.position.x + 0.5, entity.position.y + 0.5), {0, 0}, colors::red};
    // ColoredVertex B{std::make_shared<Vec2Float>(entity.nextStep.x + 0.5, entity.nextStep.y + 0.5), {0, 0}, colors::red};
    // std::vector<ColoredVertex> line{A, B};
    // this->debugData.emplace_back(new DebugData::Primitives(line, PrimitiveType::LINES));

    ColoredVertex C{std::make_shared<Vec2Float>(entity.position.x + 0.5, entity.position.y + 0.5), {0, 0}, colors::green};
    ColoredVertex D{std::make_shared<Vec2Float>(entity.target.x + 0.5, entity.target.y + 0.5), {0, 0}, colors::green};
    std::vector<ColoredVertex> line2{C, D};
    this->debugData.emplace_back(new DebugData::Primitives(line2, PrimitiveType::LINES));
}

void MyStrategy::getRangedUnitIntention(Entity& entity, 
                                        std::vector<std::vector<int>>& mapOccupied,
                                        std::vector<std::vector<int>>& mapDamage)
{

}

void MyStrategy::getMeleeUnitIntention(Entity& entity, 
                                       std::vector<std::vector<int>>& mapOccupied,
                                       std::vector<std::vector<int>>& mapDamage)
{

}

void MyStrategy::getBuilderUnitMove(Entity& entity, 
                                    std::vector<std::vector<int>>& mapOccupied,
                                    std::vector<std::vector<int>>& mapDamage, 
                                    std::vector<std::vector<Vec2Int>>& mapCameFrom)
{
    bool isCameFromCollision = false;
    Vec2Int cameFrom;
    int tmp;
    if (mapOccupied[entity.target.x][entity.target.y] != -1)
    {
        getBuilderUnitIntention(entity, mapOccupied, mapDamage);
    }
    if (mapOccupied[entity.position.x][entity.position.y] != -1)
    {
        isCameFromCollision = true;
        cameFrom = mapCameFrom[entity.position.x][entity.position.y];
        tmp = mapOccupied[cameFrom.x][cameFrom.y];
        mapOccupied[cameFrom.x][cameFrom.y] = EntityType::MELEE_UNIT;
    }
    if (mapOccupied[entity.nextStep.x][entity.nextStep.y] != -1)
    {
        if (entity.target == Vec2Int{-1, -1})
        {
            Vec2Int retreatPos = getRetreatPos(entity.position, mapDamage, mapOccupied);
            entity.nextStep = retreatPos;
        }
        else
        {
            WayPoint point = entity.astar(entity.target, mapOccupied);
        }       
    }
    if (isCameFromCollision)
    {
        mapOccupied[cameFrom.x][cameFrom.y] = tmp;
    }
    mapOccupied[entity.nextStep.x][entity.nextStep.y] = entity.entityType;
    mapCameFrom[entity.nextStep.x][entity.nextStep.y] = entity.position;
}

void MyStrategy::getRangedUnitMove(Entity& entity, std::vector<std::vector<int>>& mapOccupied, std::vector<std::vector<int>>& mapDamage, std::vector<std::vector<Vec2Int>>& mapCameFrom)
{

}

void MyStrategy::getMeleeUnitMove(Entity& entity, std::vector<std::vector<int>>& mapOccupied, std::vector<std::vector<int>>& mapDamage, std::vector<std::vector<Vec2Int>>& mapCameFrom)
{

}

EntityAction MyStrategy::getBuilderUnitAction(Entity& entity)
{
    EntityAction action;
    if (entity.nextStep != entity.position)
    {
        MoveAction move;
        move.target = entity.nextStep;
        move.breakThrough = false;
        move.findClosestPosition = false;
        action.moveAction = std::make_shared<MoveAction>(move);

        // ColoredVertex A{std::make_shared<Vec2Float>(entity.position.x + 0.5, entity.position.y + 0.5), {0, 0}, colors::white};
        // ColoredVertex B{std::make_shared<Vec2Float>(entity.nextStep.x + 0.5, entity.nextStep.y + 0.5), {0, 0}, colors::white};
        // std::vector<ColoredVertex> line{A, B};
        // this->debugData.emplace_back(new DebugData::Primitives(line, PrimitiveType::LINES));
    }
    else
    {
        if (entity.attackTarget)
        {
            AttackAction attack;
            attack.target = std::make_shared<int>((*entity.attackTarget).id);
            action.attackAction = std::make_shared<AttackAction>(attack);
        }
        else if (entity.repairTarget)
        {
            RepairAction repair;
            repair.target = (*entity.repairTarget).id;
            action.repairAction = std::make_shared<RepairAction>(repair);

            ColoredVertex A{std::make_shared<Vec2Float>(entity.position.x, entity.position.y), {0, 0}, colors::green};
            ColoredVertex B{std::make_shared<Vec2Float>((*entity.repairTarget).position.x, (*entity.repairTarget).position.y), {0, 0}, colors::green};
            std::vector<ColoredVertex> line{A, B};
            this->debugData.emplace_back(new DebugData::Primitives(line, PrimitiveType::LINES));
        }
        else if (entity.buildTarget)
        {
            BuildAction build;
            build.position = (*entity.buildTarget).position;
            build.entityType = (*entity.buildTarget).entityType;
            action.buildAction = std::make_shared<BuildAction>(build);
        }
    } 
    return action;
}

EntityAction MyStrategy::getRangedUnitAction(Entity& entity)
{
    EntityAction action;
    return action;
}

EntityAction MyStrategy::getMeleeUnitAction(Entity& entity)
{   
    EntityAction action;
    return action;
}

std::vector<Entity> MyStrategy::findFreePosOnBuildCellMap(std::vector<std::vector<int>>& map, Entity building)
{
    std::vector<Entity> positions;
    int mapSize = map[0].size();
    int size = building.getSize();
    int limit = mapSize - size;
    int step = 1;
    for (auto pair : buildOrder)
        fillMapCells(map, pair.second.position, 1, size, 1);

    for (int i = 0; i < limit; i+=step)
        for (int j = 0; j < limit; j+=step)
        {
            bool freeArea = true;
            for (int n = i; n < i + size; n++)
            {
                for (int m = j; m < j + size; m++)
                {
                    // std::cout<<"freePosToBuild: "<<n<<", "<<m<<std::endl;
                    if (map[n][m] != 0)
                    {
                        freeArea = false;
                        break;
                    }
                }
                    
                if (freeArea == false)
                    break;
            }
            if (freeArea == true)
            {
                building.position = Vec2Int{i, j};
                positions.emplace_back(building);
            } 
        }
    return positions;
}

void MyStrategy::fillBuildOrder(std::vector<std::vector<int>>& mapBuilding, std::vector<std::vector<int>>& mapOccupied, std::unordered_map<int, EntityAction>& orders)
{
    if (buildOrder.size() < 1)
    {
        if (countBase == 0 && myAvailableResources >= prices::builderBase)
        {
            Entity builderBase;
            builderBase.id = -1;
            builderBase.entityType = EntityType::BUILDER_BASE;
            std::vector<Entity> possibleBuildPositions = findFreePosOnBuildCellMap(mapBuilding, builderBase);
            Entity builder = findNearestFreeBuilder(baseCenter, orders);
            if (builder.id != -1 && possibleBuildPositions.size() > 0)
            {
                Entity base = findNearestEntity(builder, possibleBuildPositions, mapOccupied, true);
                if (base.position != Vec2Int{-1, -1})
                {
                    buildOrder[builder.id] = base;
                }
            }
        }
        else if (countRangeBase < 2 && myAvailableResources >= prices::rangeBase)
        {
            Entity rangeBase;
            rangeBase.id = -1;
            rangeBase.entityType = EntityType::RANGED_BASE;
            std::vector<Entity> possibleBuildPositions = findFreePosOnBuildCellMap(mapBuilding, rangeBase);
            Entity builder = findNearestFreeBuilder(baseCenter, orders);
            if (builder.id != -1 && possibleBuildPositions.size() > 0)
            {
                Entity base = findNearestEntity(builder, possibleBuildPositions, mapOccupied, true);
                if (base.position != Vec2Int{-1, -1})
                {
                    buildOrder[builder.id] = base;
                }
            }
        }
        else if (myAvailablePopulation < 7 && myAvailableResources >= prices::house)
        {
            Entity house;
            house.id = -1;
            house.entityType = EntityType::HOUSE;
            std::vector<Entity> possibleBuildPositions = findFreePosOnBuildCellMap(mapBuilding, house);
            Entity builder = findNearestFreeBuilder(baseCenter, orders);
            if (builder.id != -1 && possibleBuildPositions.size() > 0)
            {
                Entity base = findNearestEntity(builder, possibleBuildPositions, mapOccupied, true);
                if (base.position != Vec2Int{-1, -1})
                {
                    buildOrder[builder.id] = base;
                }
            }
        }
    }
}

Entity MyStrategy::findNearestFreeBuilder(Entity& entity, std::unordered_map<int, EntityAction>& orders)
{
    int min_dist = 100000;
    Entity nearestEntity;
    nearestEntity.id = -1;
    for (Entity e : myUnits)
    {
        if (e.entityType == EntityType::BUILDER_UNIT)
        {
            int d = distance(entity.position, e.position);
            bool isFree = true;
            if (buildOrder.find(e.id) != buildOrder.end()) isFree = false;
            if (orders[e.id].repairAction || orders[e.id].buildAction) isFree = false;
            if (isFree && d < min_dist) 
            {
                min_dist = d;
                nearestEntity = e;
            }
        }
    }
    return nearestEntity;
}

void MyStrategy::delImposibleOrders(std::vector<std::vector<int>>& mapBuilding)
{
    std::unordered_map<int, Entity> newBuildOrder;
    
    for (auto pair : buildOrder)
    {
        bool isImposible = false;
        int size = pair.second.getSize();
        for (int j = pair.second.position.x; j < pair.second.position.x + size; j++)
        {
            for (int k = pair.second.position.y; k < pair.second.position.y + size; k++)
            {
                if (mapBuilding[j][k] != 0)
                {
                    isImposible = true;
                    cout<<"Impossible build!"<<endl;
                    break;
                }
            }
            if (isImposible) break;
        }
        
        if (!isImposible)
        {
            newBuildOrder[pair.first] = pair.second;
        }
    }
    cout<<"new BO size (impos): "<<newBuildOrder.size()<<endl;
    buildOrder = newBuildOrder;
}

void MyStrategy::delDeadUnitsFromBuildOrder()
{
    std::unordered_map<int, Entity> newBuildOrder;

    for (auto pair : buildOrder)
    {
        bool isDead = true;
        for (Entity entity : myUnits)
            if(entity.id == pair.first)
            {
                isDead = false;
                cout<<"Dead builder!"<<endl;
                break;
            }
        if (!isDead)
        {
            newBuildOrder[pair.first] = pair.second;
        }
    }
    cout<<"new BO size (dead): "<<newBuildOrder.size()<<endl;
    buildOrder = newBuildOrder;
}