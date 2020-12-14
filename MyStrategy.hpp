#ifndef _MY_STRATEGY_HPP_
#define _MY_STRATEGY_HPP_

#include "DebugInterface.hpp"
#include "model/Model.hpp"
#include "utils.hpp"

class MyStrategy {
public:
    MyStrategy();
    Action getAction(const PlayerView playerView, DebugInterface* debugInterface);
    void debugUpdate(const PlayerView& playerView, DebugInterface& debugInterface);

    bool isGlobalsSet = false;
    int myAvailableResources = 0;
    int myAvailablePopulation = 0;

    int lastAvailableResources = 0;
    int d_Resources = 0;

    int countRangeUnits = 0;
    int countMeleeUnits = 0;
    int countBuilderUnits = 0;
    
    int potentialPopulation;

    int countBase = 0;
    int countRangeBase = 0;
    int countMeleBase = 0;

    Entity remontPoint;
    Entity baseCenter;
    Entity enemyCenter1;
    Entity enemyCenter2;
    Entity enemyCenter3;

    std::vector<Entity> resourses, 
                        enemyEntities, 
                        myUnits, 
                        myTurrets,
                        myBuildings, 
                        myDamagedBuildings;

    std::unordered_map<EntityType, EntityProperties> entityProperties;
    std::vector<Entity> busyUnits;
    std::vector<Entity> buildOrder;

    std::vector <std::shared_ptr<DebugData>> debugData;

    void setGlobals(const PlayerView& playerView);
    int distance(Entity& e1, Entity& e2);
    EntityAction chooseBuilderUnitAction(Entity& entity, 
                                         std::vector<std::vector<int>>& mapOccupied,
                                         std::vector<std::vector<int>>& mapDamage);
    void fillBuildOrder(std::vector<std::vector<int>>& mapBuilding, std::unordered_map<int, EntityAction>& orders);
    EntityAction chooseRecruitUnitAction(Entity& entity, const PlayerView& playerView, int enemyDistToBase, std::vector<std::vector<int>>& mapOccupied);
    EntityAction chooseRangeUnitAction(Entity& entity,
                                        std::vector<std::vector<int>>& mapOccupied, 
                                        std::vector<std::vector<int>>& mapAlly, 
                                        std::vector<std::vector<int>>& mapEnemy,
                                        std::vector<std::vector<int>>& mapDamage);
    EntityAction chooseMeleeUnitAction(Entity& entity,
                                        std::vector<std::vector<int>>& mapOccupied, 
                                        std::vector<std::vector<int>>& mapAlly, 
                                        std::vector<std::vector<int>>& mapEnemy,
                                        std::vector<std::vector<int>>& mapDamage);
    ConstructAction constructHouse(Vec2Int buildingPosition, std::vector<std::vector<int>>& map);
    Entity findNearestEntity(Entity& entity, std::vector<Entity>& entities);
    Entity findNearestFreeBuilder(Entity& entity, std::unordered_map<int, EntityAction>& orders);
    std::vector<Entity> findFreePosOnBuildCellMap(std::vector<std::vector<int>>& map, EntityType type);
    Vec2Int findClosestFreePosNearBuilding(Entity& entity, Entity& building, std::vector<std::vector<int>>& mapOccupied);
    void delDeadUnitsFromBuildOrder();
    void delImposibleOrders(std::vector<std::vector<int>>& mapBuilding);
    Vec2Int findPosNearBuilding(Entity& entity, Entity& building);
    Vec2Int getNextStep(Entity entity, Entity target, std::vector<std::vector<int>>& mapOccupied);

    std::vector<Entity> filterFreeResources(std::vector<Entity>& resourses, std::vector<std::vector<int>>& mapResourse);
    Entity findNearestResourse(Entity& entity, std::vector<Entity>& entities);
};

#endif