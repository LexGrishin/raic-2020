#include "Entity.hpp"

Entity::Entity() { }
Entity::Entity(int id, std::shared_ptr<int> playerId, EntityType entityType, Vec2Int position, int health, bool active) : id(id), playerId(playerId), entityType(entityType), position(position), health(health), active(active) { }
Entity Entity::readFrom(InputStream& stream) {
    Entity result;
    result.id = stream.readInt();
    if (stream.readBool()) {
        result.playerId = std::shared_ptr<int>(new int());
        *result.playerId = stream.readInt();
    } else {
        result.playerId = std::shared_ptr<int>();
    }
    switch (stream.readInt()) {
    case 0:
        result.entityType = EntityType::WALL;
        break;
    case 1:
        result.entityType = EntityType::HOUSE;
        break;
    case 2:
        result.entityType = EntityType::BUILDER_BASE;
        break;
    case 3:
        result.entityType = EntityType::BUILDER_UNIT;
        break;
    case 4:
        result.entityType = EntityType::MELEE_BASE;
        break;
    case 5:
        result.entityType = EntityType::MELEE_UNIT;
        break;
    case 6:
        result.entityType = EntityType::RANGED_BASE;
        break;
    case 7:
        result.entityType = EntityType::RANGED_UNIT;
        break;
    case 8:
        result.entityType = EntityType::RESOURCE;
        break;
    case 9:
        result.entityType = EntityType::TURRET;
        break;
    default:
        throw std::runtime_error("Unexpected tag value");
    }
    result.position = Vec2Int::readFrom(stream);
    result.health = stream.readInt();
    result.active = stream.readBool();
    result.distToTarget = 100000;
    return result;
}
void Entity::writeTo(OutputStream& stream) const {
    stream.write(id);
    if (playerId) {
        stream.write(true);
        stream.write((*playerId));
    } else {
        stream.write(false);
    }
    stream.write((int)(entityType));
    position.writeTo(stream);
    stream.write(health);
    stream.write(active);
}

// -------------------------------------------------------------------------------------------------------------------------------

Vec2Int Entity::getDockingPos(Entity& entity, std::vector<std::vector<int>>& mapOccupied)
{  
    int size = entityProperties[building.entityType].size;
    int mapSize = mapOccupied[0].size();
    int min_dist = 100000;
    Vec2Int min_pos{-1, -1};

    if (building.position.x > 0)
        for (int y = building.position.y; y < building.position.y + size; y++)
        {
            if (mapOccupied[building.position.x - 1][y] == 0)
            {
                Entity e;
                e.entityType = EntityType::BUILDER_UNIT;
                e.position = Vec2Int{building.position.x - 1, y};
                int dist = distance(entity, e);
                if (dist < min_dist)
                {
                    min_dist = dist;
                    min_pos = e.position;
                }
            }
        }
    if (building.position.x + size < mapSize)
        for (int y = building.position.y; y < building.position.y + size; y++)
        {
            if (mapOccupied[building.position.x + size][y] == 0)
            {
                Entity e;
                e.entityType = EntityType::BUILDER_UNIT;
                e.position = Vec2Int{building.position.x + size, y};
                int dist = distance(entity, e);
                if (dist < min_dist)
                {
                    min_dist = dist;
                    min_pos = e.position;
                }
            }
        }
    if (building.position.y + size < mapSize)
        for (int x = building.position.x; x < building.position.x + size; x++)
        {
            if (mapOccupied[x][building.position.y + size] == 0)
            {
                Entity e;
                e.entityType = EntityType::BUILDER_UNIT;
                e.position = Vec2Int{x, building.position.y + size};
                int dist = distance(entity, e);
                if (dist < min_dist)
                {
                    min_dist = dist;
                    min_pos = e.position;
                }
            }
        }
    if (building.position.y > 0)
        for (int x = building.position.x; x < building.position.x + size; x++)
        {
            if (mapOccupied[x][building.position.y - 1] == 0)
            {
                Entity e;
                e.entityType = EntityType::BUILDER_UNIT;
                e.position = Vec2Int{x, building.position.y - 1};
                int dist = distance(entity, e);
                if (dist < min_dist)
                {
                    min_dist = dist;
                    min_pos = e.position;
                }
            }
        }
    return min_pos;
}
