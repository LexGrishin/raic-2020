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
    Entity defaultTarget;
    Entity remontPoint;

    std::vector<Entity> resourses, 
                        availableResourses,
                        enemyEntities, 
                        myBuiderUnits, myAttackUnits, myTurrets,
                        myBuildings, myDamagedBuildings,
                        possibleBuildPositions;

    std::unordered_map<EntityType, EntityProperties> entityProperties;

    std::vector <std::shared_ptr<DebugData>> debugData;

    void setGlobals(const PlayerView& playerView);
    int distance(Entity& e1, Entity& e2);
    // float euclideanDist(Entity& e1, Entity& e2);
    EntityAction chooseBuilderUnitAction(Entity& entity, const PlayerView& playerView, std::vector<std::vector<int>>& map);
    EntityAction chooseRecruitUnitAction(Entity& entity, const PlayerView& playerView);
    EntityAction chooseAttackUnitAction(Entity& entity,
                                        std::vector<std::vector<int>>& mapOccupied, 
                                        std::vector<std::vector<int>>& mapAlly, 
                                        std::vector<std::vector<int>>& mapEnemy,
                                        std::vector<std::vector<int>>& mapDamage);
    ConstructAction constructHouse(Vec2Int buildingPosition, std::vector<std::vector<int>>& map);
    Entity findNearestEntity(Entity& entity, std::vector<Entity>& entities, std::vector<std::vector<int>>& map, bool ignoreAvailable);
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