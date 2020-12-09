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
    int atackUnitOrder = 0;
    int builderUnitOrder = 0;
    int constructOrder = 0;
    int lastAvailableResources = 0;
    int d_Resources = 0;
    int countRangeUnits = 0;
    int countMeleeUnits = 0;
    int countBuilderUnits = 0;
    int potentialPopulation;
    Entity defaultTarget;
    Entity remontPoint;
    Entity baseCenter;

    std::vector<Entity> resourses, 
                        availableResourses,
                        enemyEntities, 
                        enemyBuilders,
                        myBuilderUnits, myAttackUnits, myTurrets,
                        myBuildings, myDamagedBuildings;
                        // possibleBuildPositions;

    std::unordered_map<EntityType, EntityProperties> entityProperties;
    std::vector<Entity> busyUnits;
    std::vector<Entity> buildOrder;
    std::vector<int> buildStage;

    std::vector <std::shared_ptr<DebugData>> debugData;

    void setGlobals(const PlayerView& playerView);
    int distance(Entity& e1, Entity& e2);
    // float euclideanDist(Entity& e1, Entity& e2);
    EntityAction chooseBuilderUnitAction(Entity& entity, 
                                         std::vector<std::vector<int>>& mapOccupied,
                                         std::vector<std::vector<int>>& mapDamage);
    void fillBuildOrder(std::vector<std::vector<int>>& mapBuilding, std::unordered_map<int, EntityAction>& orders);
    EntityAction chooseRecruitUnitAction(Entity& entity, const PlayerView& playerView, int enemyDistToBase);
    EntityAction chooseRangeUnitAction(Entity& entity,
                                        std::vector<std::vector<int>>& mapOccupied, 
                                        std::vector<std::vector<int>>& mapAlly, 
                                        std::vector<std::vector<int>>& mapEnemy,
                                        std::vector<std::vector<int>>& mapDamage,
                                        int enemyDistToBase);
    EntityAction chooseMeleeUnitAction(Entity& entity,
                                        std::vector<std::vector<int>>& mapOccupied, 
                                        std::vector<std::vector<int>>& mapAlly, 
                                        std::vector<std::vector<int>>& mapEnemy,
                                        std::vector<std::vector<int>>& mapDamage,
                                        int enemyDistToBase);
    ConstructAction constructHouse(Vec2Int buildingPosition, std::vector<std::vector<int>>& map);
    Entity findNearestEntity(Entity& entity, std::vector<Entity>& entities, std::vector<std::vector<int>>& map, bool ignoreAvailable);
    Entity findNearestFreeBuilder(Entity& entity, std::unordered_map<int, EntityAction>& orders);
    std::vector<Entity> findFreePosOnBuildCellMap(std::vector<std::vector<int>>& map, EntityType type);
    void delDeadUnitsFromBuildOrder();
    Vec2Int findPosNearBuilding(Entity& entity, Entity& building);
    // Vec2Int findPosNearEntity
    // Entity findNearestReachableResource(Entity& entity, std::unordered_map<int, Entity>& entities);
    // EntityAction getAttackUnitAction(Entity& entity, 
    //                                  std::vector<std::vector<int>>& mapAttack, 
    //                                  std::vector<std::vector<int>>& mapDamage,
    //                                  std::vector<std::vector<int>>& mapOccupied);
    // MoveAnalysis analyzeMove(Entity& entity, int dx, int dy, Entity& target,
    //                                  std::vector<std::vector<int>>& mapAttack, 
    //                                  std::vector<std::vector<int>>& mapDamage,
    //                                  std::vector<std::vector<int>>& mapOccupied);
};

#endif