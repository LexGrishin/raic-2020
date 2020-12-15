#ifndef _MODEL_ENTITY_HPP_
#define _MODEL_ENTITY_HPP_

#include "../Stream.hpp"
#include "EntityType.hpp"
#include "Vec2Int.hpp"
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <tuple>
#include <queue>
#include <unordered_set>
#include "../utils.hpp"

class Entity {
public:
    int id;
    std::shared_ptr<int> playerId;
    EntityType entityType;
    Vec2Int position;
    int health;
    bool active;
    
    Entity();
    Entity(int id, std::shared_ptr<int> playerId, EntityType entityType, Vec2Int position, int health, bool active);
    static Entity readFrom(InputStream& stream);
    void writeTo(OutputStream& stream) const;

    // -------------------------------------------------------------------------------------------------------------
    int distToTarget;
    int priority = 0;
    Vec2Int nextStep;
    int getSize();
    int populationProvide();
    int populationUse();
    int maxHealth();
    int attackRange();
    int damage();
    std::tuple<Vec2Int, int> Entity::getDockingPos(Vec2Int& requesterPos, std::vector<std::vector<int>>& mapOccupied);
    bool operator< (const Entity& entity);
    bool operator> (const Entity& entity);
    WayPoint astar(Vec2Int target, std::vector<std::vector<int>>& mapOccupied);
    Vec2Int getNextStep(WayPoint& point);
};

#endif
