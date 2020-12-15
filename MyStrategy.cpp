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
        EntityAction action;

        if (entity.entityType == EntityType::BUILDER_UNIT)
        {
            getBuilderUnitMove(entity, mapOccupied);
            action = getBuilderUnitAction(entity);
        }
        else if (entity.entityType == EntityType::RANGED_UNIT)
        {
            getRangedUnitMove(entity, mapOccupied);
            action = getRangedUnitAction(entity);
        }
        else
        {
            getMeleeUnitMove(entity, mapOccupied);
            action = getMeleeUnitAction(entity);
        }
        orders[entity.id] = action;
    }


// -----------------------------------------------------------------------------------------------------------------------------
    delDeadUnitsFromBuildOrder();
    delImposibleOrders(mapBuilding);

    for (Entity ent : buildOrder)
    {
        fillMapCells(mapBuilding, ent.position, 1, entityProperties[ent.entityType].size, 1);
    }

    cout<<"Here: 5"<<endl;
    
    // Choose turns order.
    // int n = myUnits.size();
    // for (int i = 0; i < n; i++)
    // {
    //     if (myUnits[i].entityType == EntityType::BUILDER_UNIT)
    //     {
    //         Entity nearestResource = findNearestEntity(myUnits[i], resourses, mapOccupied, false);
    //         Entity nearestDamagedBuilding = findNearestEntity(myUnits[i], myDamagedBuildings, mapOccupied, false);
    //         Entity targetConstruct;
    //         targetConstruct.id = -1;
    //         int orderLength = buildOrder.size();
    //         bool isBusy = false;
    //         int orderId = 0;
    //         for (int j = 0; j < orderLength; j++)
    //             if (myUnits[i].id == busyUnits[j].id)
    //             {
    //                 isBusy = true;
    //                 orderId = j;
    //                 break;
    //             }
    //         if (isBusy)
    //         {
    //             targetConstruct = buildOrder[orderId];
    //             myUnits[i].distToTarget = distance(myUnits[i], targetConstruct);
    //         }
    //         else
    //         {
    //             int distResource = 100000;
    //             if (nearestResource.id != -1)
    //                 distResource = distance(myUnits[i], nearestResource);
    //             int distRepair = 100000;
    //             if (nearestDamagedBuilding.id != -1) 
    //                 distRepair = distance(myUnits[i], nearestDamagedBuilding);
    //             if (distResource < distRepair)
    //                 myUnits[i].distToTarget = distResource;
    //             else
    //                 myUnits[i].distToTarget = distRepair;
    //         }
    //     }
    //     else
    //     {
    //         Entity nearestEnemy = findNearestEntity(myUnits[i], enemyEntities, mapOccupied, true);
    //         int distNearestEnemy = 100000;
    //         if (nearestEnemy.id != -1)
    //         {
    //             distNearestEnemy = distance(myUnits[i], nearestEnemy);
    //             myUnits[i].distToTarget = distNearestEnemy;
    //         }
    //     }
    // }
    // std::sort(myUnits.begin(), myUnits.end(), [](const Entity& lhs, const Entity& rhs) {return lhs.distToTarget < rhs.distToTarget;});

    resourses = filterFreeResources(resourses, mapResourse);

    cout<<"Here: 5.1"<<endl;
    // int counter = 0;
    for (Entity entity : myUnits)
    {
        EntityAction action;
        if (entity.entityType == EntityType::BUILDER_UNIT)
            action = chooseBuilderUnitAction(entity, mapOccupied, mapDamage);
        else if (entity.entityType == EntityType::RANGED_UNIT)
            action = chooseRangeUnitAction(entity, mapOccupied, mapAlly, mapEnemy, mapDamage);
        else
            action = chooseMeleeUnitAction(entity, mapOccupied, mapAlly, mapEnemy, mapDamage);
        orders[entity.id] = action;

        // this->debugData.emplace_back(new DebugData::PlacedText(ColoredVertex(std::make_shared<Vec2Float>(entity.position.x, entity.position.y), 
        //                                                                                              Vec2Float(30, 0), colors::green), 
        //                                                                     to_string(counter), 0, 20));
        // counter++;
    }

    cout<<"Here: 6"<<endl;

    Entity nearestEnemyToBase = findNearestEntity(baseCenter, enemyEntities);
    int enemyDistToBase = 100000;
    if (nearestEnemyToBase.id != -1)
        enemyDistToBase = distance(baseCenter, nearestEnemyToBase);

    for (Entity entity : myBuildings)
    {
        EntityAction action = chooseRecruitUnitAction(entity, playerView, enemyDistToBase, mapOccupied);
        orders[entity.id] = action;
    }

    cout<<"Here: 7"<<endl;

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

    fillBuildOrder(mapBuilding, orders);

    for (int i=0; i < mapBuilding[0].size(); i++)
        for (int j=0; j < mapBuilding[0].size(); j++)
        {
            if (mapBuilding[i][j] != 0)
            {
                this->debugData.emplace_back(new DebugData::PlacedText(ColoredVertex(std::make_shared<Vec2Float>(i, j), 
                                                                         Vec2Float(0, 0), colors::red), 
                                                                         to_string(mapBuilding[i][j]), 
                                                                         0, 10));
            }
        }

    float tickBalance = unitBalance(playerView.currentTick);
    float currentBalance = float(countBuilderUnits)/(float(countRangeUnits + countMeleeUnits + countBuilderUnits) + 0.000001);

    cout<<"Here: 8"<<endl;

    Color color;
    if (currentBalance < tickBalance)
        color = colors::red;
    else
        color = colors::green;
    
    // this->debugData.emplace_back(new DebugData::PlacedText(ColoredVertex(debug::info_pos, Vec2Float(0, -100), colors::white), 
    //                                                        "Available res: "+to_string(myAvailableResources), 
    //                                                                      0, 20));
    // this->debugData.emplace_back(new DebugData::PlacedText(ColoredVertex(std::make_shared<Vec2Float>(remontPoint.position.x, remontPoint.position.y), 
    //                                                                                                  Vec2Float(0, 0), colors::red), 
    //                                                        "X", 0, 20));
    // this->debugData.emplace_back(new DebugData::PlacedText(ColoredVertex(std::make_shared<Vec2Float>(baseCenter.position.x, baseCenter.position.y), 
    //                                                                                                  Vec2Float(0, 0), colors::green), 
    //                                                        "X", 0, 20));
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
        <<" BU: "<<busyUnits.size()
        <<endl;

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

EntityAction MyStrategy::chooseBuilderUnitAction(Entity& entity, std::vector<std::vector<int>>& mapOccupied, std::vector<std::vector<int>>& mapDamage)
{
    EntityAction resultAction;
    resultAction.attackAction = nullptr;
    resultAction.buildAction = nullptr;
    resultAction.repairAction = nullptr;
    resultAction.moveAction = nullptr;

    Entity nearestResource = findNearestResourse(entity, resourses);
    Entity nearestDamagedBuilding = findNearestEntity(entity, myDamagedBuildings);

    cout<<"Here: 5.1.1"<<endl;

    int radius = 2;
    int stayDamage = countDamageSum(entity.position, radius, mapDamage);

    int orderLength = buildOrder.size();
    bool isBusy = false;
    int orderId = 0;
    for (int i = 0; i < orderLength; i++)
        if (entity.id == busyUnits[i].id)
        {
            isBusy = true;
            orderId = i;
            
            this->debugData.emplace_back(new DebugData::PlacedText(ColoredVertex(std::make_shared<Vec2Float>(entity.position.x, entity.position.y), Vec2Float(0, 0), colors::red), 
                                                           "B",0, 20));
            // ColoredVertex A{std::make_shared<Vec2Float>(entity.position.x, entity.position.y), {0, 0}, colors::green};
            // ColoredVertex B{std::make_shared<Vec2Float>(buildOrder[orderId].position.x, buildOrder[orderId].position.y), {0, 0}, colors::green};
            // std::vector<ColoredVertex> line{A, B};
            // this->debugData.emplace_back(new DebugData::Primitives(line, PrimitiveType::LINES));
            break;
        }
    cout<<"Here: 5.1.2"<<endl;
    if (stayDamage > 4) // Поблизости враги! Отступаем.
    {
        MoveAction action;
        Vec2Int retreatPos = getRetreatPos(entity.position, mapDamage, mapOccupied);
        action.target = retreatPos;
        action.breakThrough = false;
        action.findClosestPosition = true;
        resultAction.moveAction = std::make_shared<MoveAction>(action);
    }
    else if (nearestDamagedBuilding.id != -1 && (!isBusy)) // Если есть поврежденные строения.
    {
        int dist = distance(entity, nearestDamagedBuilding);
        if (dist > 0)
        {
            if ((distance(entity, nearestDamagedBuilding) < 2) // Если мы в двух шагах, то идем чинить.
               || (entity.id % 10 == 0 && dist < 15))
            {
                cout<<"Here: 5.1. go repair"<<endl;
                MoveAction action;
                action.target = nearestDamagedBuilding.position;
                action.breakThrough = false;
                action.findClosestPosition = true;
                resultAction.moveAction = std::make_shared<MoveAction>(action);

                ColoredVertex A{std::make_shared<Vec2Float>(entity.position.x, entity.position.y), {0, 0}, colors::green};
                ColoredVertex B{std::make_shared<Vec2Float>(nearestDamagedBuilding.position.x, nearestDamagedBuilding.position.y), {0, 0}, colors::green};
                std::vector<ColoredVertex> line{A, B};
                this->debugData.emplace_back(new DebugData::Primitives(line, PrimitiveType::LINES));
            }
            else if (nearestResource.id != -1) // Иначе идем добывать ресурсы.
            {
                if (distance(entity, nearestResource)>=(*entityProperties[entity.entityType].attack).attackRange)
                {
                    cout<<"Here: 5.1. not repair go harvest"<<endl;
                    MoveAction action;
                    action.target = nearestResource.position;
                    action.breakThrough = false;
                    action.findClosestPosition = true;
                    resultAction.moveAction = std::make_shared<MoveAction>(action);
                    // ColoredVertex A{std::make_shared<Vec2Float>(entity.position.x, entity.position.y), {0, 0}, colors::white};
                    // ColoredVertex B{std::make_shared<Vec2Float>(action.target.x, action.target.y), {0, 0}, colors::white};
                    // std::vector<ColoredVertex> line{A, B};
                    // this->debugData.emplace_back(new DebugData::Primitives(line, PrimitiveType::LINES));
                }
                else
                {
                    cout<<"Here: 5.1. not repair - harvest!"<<endl;
                    AttackAction action;
                    action.target = std::make_shared<int>(nearestResource.id);
                    resultAction.attackAction = std::make_shared<AttackAction>(action);
                }
            } 
            else // Если нет ресурсов поблизости
            {
                cout<<"Here: 5.1. no resourses"<<endl;
                if (entity.id % 3 == 1)
                {
                    MoveAction action;
                    action.target = enemyCenter1.position;
                    action.breakThrough = true;
                    action.findClosestPosition = true;
                    resultAction.moveAction = std::make_shared<MoveAction>(action);
                }
                else if (entity.id % 3 == 2)
                {
                    MoveAction action;
                    action.target = enemyCenter2.position;
                    action.breakThrough = true;
                    action.findClosestPosition = true;
                    resultAction.moveAction = std::make_shared<MoveAction>(action);
                }
                else
                {
                    MoveAction action;
                    action.target = enemyCenter3.position;
                    action.breakThrough = true;
                    action.findClosestPosition = true;
                    resultAction.moveAction = std::make_shared<MoveAction>(action);
                }
            }
        }
        else // Если мы рядом, то чиним.
        {
            cout<<"Here: 5.1. repair"<<endl;
            RepairAction action;
            action.target = nearestDamagedBuilding.id;
            resultAction.repairAction = std::make_shared<RepairAction>(action);
        }  
    }
    else if (isBusy) // Юнит участвует в строительсве.
    {
        cout<<"Here: 5.1. busy"<<endl;
        Entity target = buildOrder[orderId];
        int dist = distance(entity, target);
        // cout<<"Distance: "<<dist<<" | "<<orderId<<" | ("<<target.position.x<<", "<<target.position.y<<")"<<endl;
        if (dist != 0) 
        {
            cout<<"Here: 5.1. busy go to target"<<endl;
            MoveAction action;
            Vec2Int buildPos = getBestPosNearEntity(entity, target, mapOccupied);
            action.target = buildPos;
            action.breakThrough = false;
            action.findClosestPosition = true;
            resultAction.moveAction = std::make_shared<MoveAction>(action);

            ColoredVertex A{std::make_shared<Vec2Float>(entity.position.x, entity.position.y), {0, 0}, colors::yellow};
            ColoredVertex B{std::make_shared<Vec2Float>(action.target.x, action.target.y), {0, 0}, colors::yellow};
            std::vector<ColoredVertex> line{A, B};
            this->debugData.emplace_back(new DebugData::Primitives(line, PrimitiveType::LINES));
        }
        else //if (buildStage[orderId] == 1) 
        {
            cout<<"Here: 5.1. busy build"<<endl;
            BuildAction action;
            action.entityType = target.entityType;
            action.position = target.position;
            resultAction.buildAction = std::make_shared<BuildAction>(action);
            buildOrder.erase(buildOrder.begin() + orderId);
            busyUnits.erase(busyUnits.begin() + orderId);
            // ColoredVertex A{std::make_shared<Vec2Float>(entity.position.x, entity.position.y), {0, 0}, colors::red};
            // ColoredVertex B{std::make_shared<Vec2Float>(action.position.x, action.position.y), {0, 0}, colors::red};
            // std::vector<ColoredVertex> line{A, B};
            // this->debugData.emplace_back(new DebugData::Primitives(line, PrimitiveType::LINES));
        }
    }
    else if (nearestResource.id != -1) // Иначе идем добывать ресурсы.
    {
        if (distance(entity, nearestResource)>=(*entityProperties[entity.entityType].attack).attackRange)
        {
            cout<<"Here: 5.1. harvest"<<endl;
            MoveAction action;
            action.target = nearestResource.position;
            action.breakThrough = false;
            action.findClosestPosition = true;
            resultAction.moveAction = std::make_shared<MoveAction>(action);
            ColoredVertex A{std::make_shared<Vec2Float>(entity.position.x, entity.position.y), {0, 0}, colors::white};
            ColoredVertex B{std::make_shared<Vec2Float>(action.target.x, action.target.y), {0, 0}, colors::white};
            std::vector<ColoredVertex> line{A, B};
            this->debugData.emplace_back(new DebugData::Primitives(line, PrimitiveType::LINES));
        }
        else
        {
            cout<<"Here: 5.1. harvest - attack"<<endl;
            AttackAction action;
            action.target = std::make_shared<int>(nearestResource.id);
            resultAction.attackAction = std::make_shared<AttackAction>(action);
        }
    } 
    else // Если нет ресурсов поблизости
    {
        cout<<"Here: 5.1. last no res"<<endl;
        if (entity.id % 3 == 1)
        {
            MoveAction action;
            action.target = enemyCenter1.position;
            action.breakThrough = true;
            action.findClosestPosition = true;
            resultAction.moveAction = std::make_shared<MoveAction>(action);
        }
        else if (entity.id % 3 == 2)
        {
            MoveAction action;
            action.target = enemyCenter2.position;
            action.breakThrough = true;
            action.findClosestPosition = true;
            resultAction.moveAction = std::make_shared<MoveAction>(action);
        }
        else
        {
            MoveAction action;
            action.target = enemyCenter3.position;
            action.breakThrough = true;
            action.findClosestPosition = true;
            resultAction.moveAction = std::make_shared<MoveAction>(action);
        }
    }
    cout<<"Here: 5.1. return "<<endl;
    return resultAction;
}

void MyStrategy::delImposibleOrders(std::vector<std::vector<int>>& mapBuilding)
{
    std::vector<Entity> newBusyUnits;
    std::vector<Entity> newBuildOrder;
    std::vector<int> newBuildStage;
    int orderLength = buildOrder.size();
    
    for (int i = 0; i < orderLength; i++)
    {
        bool isImposible = false;
        int size = entityProperties[buildOrder[i].entityType].size;
        for (int j = buildOrder[i].position.x; j < buildOrder[i].position.x + size; j++)
        {
            for (int k = buildOrder[i].position.y; k < buildOrder[i].position.y + size; k++)
            {
                if (mapBuilding[j][k] != 0)
                {
                    isImposible = true;
                    break;
                }
            }
            if (isImposible) break;
        }
        
        if (!isImposible)
        {
            newBusyUnits.emplace_back(busyUnits[i]);
            newBuildOrder.emplace_back(buildOrder[i]);
        }
    }
    busyUnits = newBusyUnits;
    buildOrder = newBuildOrder;
}

void MyStrategy::delDeadUnitsFromBuildOrder()
{
    std::vector<Entity> newBusyUnits;
    std::vector<Entity> newBuildOrder;
    std::vector<int> newBuildStage;
    int orderLength = buildOrder.size();
    for (int i = 0; i < orderLength; i++)
    {
        bool isDead = true;
        for (Entity entity : myUnits)
            if(entity.id == busyUnits[i].id)
            {
                isDead = false;
                break;
            }
        if (!isDead)
        {
            newBusyUnits.emplace_back(busyUnits[i]);
            newBuildOrder.emplace_back(buildOrder[i]);
        }
    }
    busyUnits = newBusyUnits;
    buildOrder = newBuildOrder;
}

void MyStrategy::fillBuildOrder(std::vector<std::vector<int>>& mapBuilding, std::unordered_map<int, EntityAction>& orders)
{
    if (buildOrder.size() < 1)
    {
        if (countBase == 0 && myAvailableResources >= entityProperties[EntityType::BUILDER_BASE].initialCost)
        {
            std::vector<Entity> possibleBuildPositions = findFreePosOnBuildCellMap(mapBuilding, EntityType::BUILDER_BASE);
            Entity builder = findNearestFreeBuilder(baseCenter, orders);
            if (builder.id != -1 && possibleBuildPositions.size() > 0)
            {
                Entity base = findNearestEntity(builder, possibleBuildPositions);
                if (base.id != -1)
                {
                    busyUnits.emplace_back(builder);
                    buildOrder.emplace_back(base);
                }
            }
        }
        else if (countRangeBase < 2 && myAvailableResources >= entityProperties[EntityType::RANGED_BASE].initialCost)
        {
            std::vector<Entity> possibleBuildPositions = findFreePosOnBuildCellMap(mapBuilding, EntityType::RANGED_BASE);
            Entity builder = findNearestFreeBuilder(baseCenter, orders);
            if (builder.id != -1 && possibleBuildPositions.size() > 0)
            {
                Entity base = findNearestEntity(builder, possibleBuildPositions);
                if (base.id != -1)
                {
                    busyUnits.emplace_back(builder);
                    buildOrder.emplace_back(base);
                }
            }
        }
        else if (myAvailablePopulation < 7 && myAvailableResources >= entityProperties[EntityType::HOUSE].initialCost)
        {
            std::vector<Entity> possibleBuildPositions = findFreePosOnBuildCellMap(mapBuilding, EntityType::HOUSE);
            Entity builder = findNearestFreeBuilder(baseCenter, orders);
            if (builder.id != -1 && possibleBuildPositions.size() > 0)
            {
                Entity house = findNearestEntity(builder, possibleBuildPositions);
                if (house.id != -1)
                {
                    busyUnits.emplace_back(builder);
                    buildOrder.emplace_back(house);
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
            int d = distance(entity, e);
            bool isFree = true;
            for (Entity busy : busyUnits)
                if (busy.id == e.id)
                {
                    isFree = false;
                    break;
                }
            if (orders[e.id].repairAction || orders[e.id].buildAction)
                isFree = false;
            if (isFree && d < min_dist) 
            {
                min_dist = d;
                nearestEntity = e;
            }
        }
    }
    return nearestEntity;
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
        if (((currentBalance < tickBalance) || (d_Resources < 0) || playerView.currentTick < 100) && (countBuilderUnits < 75))
        {
            Entity nearestResourse = findNearestEntity(entity, resourses);
            if (nearestResourse.id == -1)
            {
                nearestResourse.position = default_pos;
                nearestResourse.entityType = EntityType::RESOURCE;
            }
                
            Vec2Int recruit_pos = getBestPosNearEntity(nearestResourse, entity, mapOccupied);
            
            if (recruit_pos.x >= 0)
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
            Entity nearestEnemy = findNearestEntity(entity, enemyEntities);
            if (nearestEnemy.id == -1)
            {
                nearestEnemy.position = default_pos;
                nearestEnemy.entityType = EntityType::RANGED_UNIT;
            }
            Vec2Int recruit_pos = getBestPosNearEntity(nearestEnemy, entity, mapOccupied);
            if (recruit_pos.x >= 0)
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
            Entity nearestEnemy = findNearestEntity(entity, enemyEntities);
            if (nearestEnemy.id == -1)
            {
                nearestEnemy.position = default_pos;
                nearestEnemy.entityType = EntityType::RANGED_UNIT;
            }
            Vec2Int recruit_pos = getBestPosNearEntity(nearestEnemy, entity, mapOccupied);
            if (recruit_pos.x >= 0)
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

EntityAction MyStrategy::chooseRangeUnitAction(Entity& entity, 
                                                std::vector<std::vector<int>>& mapOccupied, 
                                                std::vector<std::vector<int>>& mapAlly, 
                                                std::vector<std::vector<int>>& mapEnemy,
                                                std::vector<std::vector<int>>& mapDamage)
{
    EntityAction resultAction;
    resultAction.attackAction = nullptr;
    resultAction.moveAction = nullptr;
    resultAction.buildAction = nullptr;
    resultAction.repairAction = nullptr;
    
    Entity nearestEnemy = findNearestEntity(entity, enemyEntities);

    int myAttackRange = (*entityProperties[entity.entityType].attack).attackRange;
    int nearbyEnemys = countUnitsInRadius(entity.position, 7, mapEnemy);
    int nearbyAllies = countUnitsInRadius(entity.position, 3, mapAlly);

    if (nearestEnemy.id != -1) // Враг обнаружен.
    {
        if (distance(entity, nearestEnemy) >= myAttackRange + 2) // Враг далеко.
        {
            if (countRangeUnits + countMeleeUnits > 5) // Если союзники рядом, то идем на врага.
            {
                MoveAction action;
                action.target = nearestEnemy.position;
                action.breakThrough = true;
                action.findClosestPosition = true;
                resultAction.moveAction = std::make_shared<MoveAction>(action);
            }
            else // Иначе, идем в точку сбора
            {
                MoveAction action;
                action.target = remontPoint.position;
                action.breakThrough = false;
                action.findClosestPosition = true;
                resultAction.moveAction = std::make_shared<MoveAction>(action);
            }
            

            // ColoredVertex A{std::make_shared<Vec2Float>(entity.position.x, entity.position.y), {0, 0}, colors::green};
            // ColoredVertex B{std::make_shared<Vec2Float>(nearestEnemy.position.x, nearestEnemy.position.y), {0, 0}, colors::green};
            // std::vector<ColoredVertex> line{A, B};
            // this->debugData.emplace_back(new DebugData::Primitives(line, PrimitiveType::LINES));
        }
        else if (distance(entity, nearestEnemy) >= myAttackRange) // Враг в буферной зоне. Принимаем решение наступать, ждать врага или отступить.
        {
            if (nearbyAllies >= nearbyEnemys) // У нас перевес - атакуем.
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
            else if (nearbyAllies == nearbyEnemys) // Паритет, ждем.
            {
                MoveAction action;
                action.target = entity.position;
                action.breakThrough = false;
                action.findClosestPosition = true;
                resultAction.moveAction = std::make_shared<MoveAction>(action);

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
        if (entity.id % 3 == 1)
        {
            MoveAction action;
            action.target = enemyCenter1.position;
            action.breakThrough = true;
            action.findClosestPosition = true;
            resultAction.moveAction = std::make_shared<MoveAction>(action);
        }
        else if (entity.id % 3 == 2)
        {
            MoveAction action;
            action.target = enemyCenter2.position;
            action.breakThrough = true;
            action.findClosestPosition = true;
            resultAction.moveAction = std::make_shared<MoveAction>(action);
        }
        else
        {
            MoveAction action;
            action.target = enemyCenter3.position;
            action.breakThrough = true;
            action.findClosestPosition = true;
            resultAction.moveAction = std::make_shared<MoveAction>(action);
        }
    }
    return resultAction;
}

EntityAction MyStrategy::chooseMeleeUnitAction(Entity& entity, 
                                                std::vector<std::vector<int>>& mapOccupied, 
                                                std::vector<std::vector<int>>& mapAlly, 
                                                std::vector<std::vector<int>>& mapEnemy,
                                                std::vector<std::vector<int>>& mapDamage)
{
    EntityAction resultAction;
    resultAction.attackAction = nullptr;
    resultAction.moveAction = nullptr;
    resultAction.buildAction = nullptr;
    resultAction.repairAction = nullptr;
    
    Entity nearestEnemy = findNearestEntity(entity, enemyEntities);

    if (nearestEnemy.id != -1) // Враг обнаружен.
    {
            MoveAction action;
            action.target = nearestEnemy.position;
            action.breakThrough = true;
            action.findClosestPosition = true;
            resultAction.moveAction = std::make_shared<MoveAction>(action); 
            ColoredVertex A{std::make_shared<Vec2Float>(entity.position.x, entity.position.y), {0, 0}, colors::red};
            ColoredVertex B{std::make_shared<Vec2Float>(nearestEnemy.position.x, nearestEnemy.position.y), {0, 0}, colors::red};
            std::vector<ColoredVertex> line{A, B};
            this->debugData.emplace_back(new DebugData::Primitives(line, PrimitiveType::LINES));
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

Entity MyStrategy::findNearestEntity(Entity& entity, std::vector<Entity>& entities)
{
    int min_dist = 100000;
    Entity nearestEntity;
    nearestEntity.id = -1;
    for (Entity e : entities)
    {
        int d = distance(entity, e);
        if (d >= 0 && d < min_dist)
        {
            min_dist = d;
            nearestEntity = e;
        }
    }
    return nearestEntity;
}

std::vector<Entity> MyStrategy::findFreePosOnBuildCellMap(std::vector<std::vector<int>>& map, EntityType type)
{
    std::vector<Entity> positions;
    int mapSize = map[0].size();
    int limit = mapSize - entityProperties[type].size;
    int step = 1;
    for (auto entity : buildOrder)
        fillMapCells(map, entity.position, 1, entityProperties[entity.entityType].size, 1);

    for (int i = 0; i < limit; i+=step)
        for (int j = 0; j < limit; j+=step)
        {
            bool freeArea = true;
            for (int n = i; n < i + entityProperties[type].size; n++)
            {
                for (int m = j; m < j + entityProperties[type].size; m++)
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
                Vec2Int pos{i, j};
                Entity ent;
                ent.id = 1;
                ent.position = pos;
                ent.entityType = type;
                positions.emplace_back(ent);
            } 
        }
    return positions;
}

std::vector<Entity> MyStrategy::filterFreeResources(std::vector<Entity>& resourses, std::vector<std::vector<int>>& mapResourse)
{
    std::vector<Entity> filteredRes;
    for (Entity e: resourses)
    {
        if(isAvailable(mapResourse, e.position, 1))
        {
            filteredRes.emplace_back(e);
        }
    }
    return filteredRes;
}

Entity MyStrategy::findNearestResourse(Entity& entity, std::vector<Entity>& entities)
{
    int min_dist = 100000;
    int min_i = 0;
    Entity nearestEntity;
    nearestEntity.id = -1;
    int n = entities.size();
    for (int i = 0; i < n; i++)
    {
        int d = distance(entity, entities[i]);
        if (d >= 0 && d < min_dist)
        {
            min_dist = d;
            nearestEntity = entities[i];
            min_i = i;
        }
    }
    entities.erase(entities.begin() + min_i);
    return nearestEntity;
}

void MyStrategy::getBuilderUnitIntention(Entity& entity, 
                                         std::vector<std::vector<int>>& mapOccupied,
                                         std::vector<std::vector<int>>& mapDamage)
{

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

void MyStrategy::getBuilderUnitMove(Entity& entity, std::vector<std::vector<int>>& mapOccupied)
{

}

void MyStrategy::getRangedUnitMove(Entity& entity, std::vector<std::vector<int>>& mapOccupied)
{

}

void MyStrategy::getMeleeUnitMove(Entity& entity, std::vector<std::vector<int>>& mapOccupied)
{

}

EntityAction MyStrategy::getBuilderUnitAction(Entity& entity)
{
    EntityAction action;
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