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

std::tuple<Vec2Int, int> Entity::getDockingPos(Vec2Int& requesterPos, std::vector<std::vector<int>>& mapOccupied)
{  
    int size = getSize();
    int mapSize = mapOccupied[0].size();

    int dockingDist = 2*mapSize;
    Vec2Int dockingPos{-1, -1};

    if (position.x > 0)
        for (int y = position.y; y < position.y + size; y++)
        {
            if (mapOccupied[position.x - 1][y] == 0)
            {
                Vec2Int testPos{position.x - 1, y};
                int dist = distance(testPos, requesterPos);
                if (dist < dockingDist)
                {
                    dockingDist = dist;
                    dockingPos = testPos;
                }
            }
        }
    if (position.x + size < mapSize)
        for (int y = position.y; y < position.y + size; y++)
        {
            if (mapOccupied[position.x + size][y] == 0)
            {
                Vec2Int testPos{position.x + size, y};
                int dist = distance(testPos, requesterPos);
                if (dist < dockingDist)
                {
                    dockingDist = dist;
                    dockingPos = testPos;
                }
            }
        }
    if (position.y + size < mapSize)
        for (int x = position.x; x < position.x + size; x++)
        {
            if (mapOccupied[x][position.y + size] == 0)
            {
                Vec2Int testPos{x, position.y + size};
                int dist = distance(testPos, requesterPos);
                if (dist < dockingDist)
                {
                    dockingDist = dist;
                    dockingPos = testPos;
                }
            }
        }
    if (position.y > 0)
        for (int x = position.x; x < position.x + size; x++)
        {
            if (mapOccupied[x][position.y - 1] == 0)
            {
                Vec2Int testPos{x, position.y - 1};
                int dist = distance(testPos, requesterPos);
                if (dist < dockingDist)
                {
                    dockingDist = dist;
                    dockingPos = testPos;
                }
            }
        }
    return {dockingPos, dockingDist};
}

int Entity::getSize()
{
    int size = 1;
    if (entityType == EntityType::BUILDER_BASE 
        || entityType == EntityType::RANGED_BASE
        || entityType == EntityType::MELEE_BASE)
    {
        size = 5;
    }
    else if (entityType == EntityType::BUILDER_UNIT
             || entityType == EntityType::RANGED_UNIT
             || entityType == EntityType::MELEE_UNIT
             || entityType == EntityType::RESOURCE
             || entityType == EntityType::WALL)
    {
        size = 1;
    }
    else if (entityType == EntityType::HOUSE)
    {
        size = 3;
    }
    else if (entityType == EntityType::TURRET)
    {
        size = 2;
    }
    return size;
}

int Entity::populationProvide()
{
    int provide = 0;
    if (entityType == EntityType::BUILDER_BASE 
        || entityType == EntityType::RANGED_BASE
        || entityType == EntityType::MELEE_BASE
        || entityType == EntityType::HOUSE)
    {
        provide = 5;
    }
    else 
    {
        provide = 0;
    }
    return provide;
}

int Entity::populationUse()
{
    int use = 1;
    if (entityType == EntityType::BUILDER_BASE 
        || entityType == EntityType::RANGED_BASE
        || entityType == EntityType::MELEE_BASE
        || entityType == EntityType::HOUSE
        || entityType == EntityType::TURRET
        || entityType == EntityType::WALL
        || entityType == EntityType::RESOURCE)
    {
        use = 0;
    }
    else 
    {
        use = 1;
    }
    return use;
}

int Entity::maxHealth()
{
    int health = 0;
    switch (entityType)
    {
        case EntityType::BUILDER_BASE:
            health = 300;
            break;
        case EntityType::MELEE_BASE:
            health = 300;
            break;
        case EntityType::RANGED_BASE:
            health = 300;
            break;
        case EntityType::BUILDER_UNIT:
            health = 10;
            break;
        case EntityType::RANGED_UNIT:
            health = 10;
            break;
        case EntityType::MELEE_UNIT:
            health = 50;
            break;
        case EntityType::TURRET:
            health = 100;
            break;
        case EntityType::RESOURCE:
            health = 30;
            break;
        case EntityType::WALL:
            health = 50;
            break;
        case EntityType::HOUSE:
            health = 50;
            break;
    }
    return health;
}

int Entity::attackRange()
{
    int range = 0;
    switch (entityType)
    {
        case EntityType::BUILDER_BASE:
            range = 0;
            break;
        case EntityType::MELEE_BASE:
            range = 0;
            break;
        case EntityType::RANGED_BASE:
            range = 0;
            break;
        case EntityType::BUILDER_UNIT:
            range = 1;
            break;
        case EntityType::RANGED_UNIT:
            range = 5;
            break;
        case EntityType::MELEE_UNIT:
            range = 1;
            break;
        case EntityType::TURRET:
            range = 5;
            break;
        case EntityType::RESOURCE:
            range = 0;
            break;
        case EntityType::WALL:
            range = 0;
            break;
        case EntityType::HOUSE:
            range = 0;
            break;
    }
    return range;
}

int Entity::damage()
{
    int dmg = 0;
    switch (entityType)
    {
        case EntityType::BUILDER_BASE:
            dmg = 0;
            break;
        case EntityType::MELEE_BASE:
            dmg = 0;
            break;
        case EntityType::RANGED_BASE:
            dmg = 0;
            break;
        case EntityType::BUILDER_UNIT:
            dmg = 1;
            break;
        case EntityType::RANGED_UNIT:
            dmg = 5;
            break;
        case EntityType::MELEE_UNIT:
            dmg = 5;
            break;
        case EntityType::TURRET:
            dmg = 5;
            break;
        case EntityType::RESOURCE:
            dmg = 0;
            break;
        case EntityType::WALL:
            dmg = 0;
            break;
        case EntityType::HOUSE:
            dmg = 0;
            break;
    }
    return dmg;
}