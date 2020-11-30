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
    std::unordered_map<EntityType, EntityProperties> entityProperties;

    std::vector <std::shared_ptr<DebugData>> debugData;

    void setGlobals(const PlayerView& playerView);
    EntityAction chooseBuilderUnitAction(Entity& entity, const PlayerView& playerView, std::vector<Entity>& resoures);
    Entity findNearestEntity(Entity& entity, std::vector<Entity>& entities);
};

#endif