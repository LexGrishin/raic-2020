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

    std::vector<Entity> resourses, 
                   enemyBuilderUnits, enemyAtackUnits, enemyBuildings, enemyHouses,
                   myBuiderUnits, myAtackUnits, myBuildings, myHouses;

    std::unordered_map<EntityType, EntityProperties> entityProperties;

    std::vector <std::shared_ptr<DebugData>> debugData;

    void setGlobals(const PlayerView& playerView);
    int distance(Entity& e1, Entity& e2);
    EntityAction chooseBuilderUnitAction(Entity& entity, const PlayerView& playerView);
    EntityAction chooseRecruitUnitAction(Entity& entity, const PlayerView& playerView);
    EntityAction chooseAtackUnitAction(Entity& entity, const PlayerView& playerView);
    Entity findNearestEntity(Entity& entity, std::vector<Entity>& entities);
};

#endif