#ifndef _UTILS_HPP_
#define _UTILS_HPP_

#include <iostream>
#include <string>
#include <math.h>

#include "model/Model.hpp"

namespace colors 
{
  const Color red{1, 0, 0, 1};
  const Color green{0, 1, 0, 1};
  const Color white{1, 1, 1, 1};
  const Color yellow{1, 1, 0, 1};
}


namespace healths
{
    const int resourse = 30;
}


namespace debug
{
    const auto info_pos = std::make_shared<Vec2Float>(80, 80);
}

const int EMPTY_OR_UNKNOWN = -1;

struct PlayerPopulation
{
    int inUse;
    int available;
};

struct ConstructAction
{
    int entityId;
    EntityAction entityAction;
};

struct MoveAnalysis
{
    int canGo;
    float distanceToTarget;
    float distanceToAlly;
    int maxDamage;
    float total;
};

class WayPoint
{
public:
    int g;
    int h;
    int f;
    std::shared_ptr<WayPoint> parent;
    Vec2Int position;

    WayPoint(Vec2Int position, int g, int h, std::shared_ptr<WayPoint> parent);

    bool operator< (const WayPoint& wayPoint);
    int id();
};

int distance(Vec2Int& a, Vec2Int& b);

float unitBalance(float x);

void fillMapCells(std::vector<std::vector<int>>& map, Vec2Int pos, int value, int size, int padding);
void fillDamageMap(std::vector<std::vector<int>>& map, Vec2Int pos, int radius, int attack);
Vec2Int getRetreatPos(Vec2Int pos, std::vector<std::vector<int>>& mapDamage, std::vector<std::vector<int>>& mapOccupied);
int countUnitsInRadius(Vec2Int pos, int radius, std::vector<std::vector<int>>& map);
int countDamageSum(Vec2Int pos, int radius, std::vector<std::vector<int>>& map);

bool isAvailable(std::vector<std::vector<int>>& map, Vec2Int pos, int size);
bool isPassable(int var);
bool exists(std::vector<std::vector<int>>& map, Vec2Int pos);

#endif