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

    std::priority_queue<Entity> myUnitsPriority;

    std::unordered_map<int, Entity> buildOrder;

    std::vector <std::shared_ptr<DebugData>> debugData;

    void setGlobals(const PlayerView& playerView);
    EntityAction chooseBuilderUnitAction(Entity& entity, 
                                         std::vector<std::vector<int>>& mapOccupied,
                                         std::vector<std::vector<int>>& mapDamage);
    
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
    
    
    Vec2Int getBestPosNearEntity(Entity& entity, Entity& building, std::vector<std::vector<int>>& mapOccupied);
    void delDeadUnitsFromBuildOrder();
    void delImposibleOrders(std::vector<std::vector<int>>& mapBuilding);
    Vec2Int findPosNearBuilding(Entity& entity, Entity& building);
    Vec2Int getNextStep(Entity entity, Entity target, std::vector<std::vector<int>>& mapOccupied);

    std::vector<Entity> filterFreeResources(std::vector<Entity>& resourses, std::vector<std::vector<int>>& mapResourse);

    // --------------------------------------------------------------------------
    std::vector<Entity> findFreePosOnBuildCellMap(std::vector<std::vector<int>>& map, Entity building);

    void getBuilderUnitIntention(Entity& entity, 
                                 std::vector<std::vector<int>>& mapOccupied,
                                 std::vector<std::vector<int>>& mapDamage);
    void getRangedUnitIntention(Entity& entity, 
                                std::vector<std::vector<int>>& mapOccupied,
                                std::vector<std::vector<int>>& mapDamage);
    void getMeleeUnitIntention(Entity& entity, 
                               std::vector<std::vector<int>>& mapOccupied,
                               std::vector<std::vector<int>>& mapDamage);
    
    void getBuilderUnitMove(Entity& entity, std::vector<std::vector<int>>& mapOccupied, std::vector<std::vector<int>>& mapDamage, std::vector<std::vector<Vec2Int>>& mapCameFrom);
    void getRangedUnitMove(Entity& entity, std::vector<std::vector<int>>& mapOccupied, std::vector<std::vector<int>>& mapDamage, std::vector<std::vector<Vec2Int>>& mapCameFrom);
    void getMeleeUnitMove(Entity& entity, std::vector<std::vector<int>>& mapOccupied, std::vector<std::vector<int>>& mapDamage, std::vector<std::vector<Vec2Int>>& mapCameFrom);

    EntityAction getBuilderUnitAction(Entity& entity);
    EntityAction getRangedUnitAction(Entity& entity);
    EntityAction getMeleeUnitAction(Entity& entity);

    Entity findNearestEntity(Entity& entity, std::vector<Entity>& entities, std::vector<std::vector<int>>& mapOccupied, bool direct);
    void fillBuildOrder(std::vector<std::vector<int>>& mapBuilding, std::vector<std::vector<int>>& mapOccupied, std::unordered_map<int, EntityAction>& orders);
    Entity findNearestFreeBuilder(Entity& entity, std::unordered_map<int, EntityAction>& orders);
};

#endif