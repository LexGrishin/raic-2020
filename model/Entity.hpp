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
    int Entity::getSize();
    std::tuple<Vec2Int, int> Entity::getDockingPos(Vec2Int& requesterPos, std::vector<std::vector<int>>& mapOccupied);

};

#endif
