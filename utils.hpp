#ifndef _UTILS_HPP_
#define _UTILS_HPP_

#include <iostream>
#include <string>

#include "model/Model.hpp"

namespace colors 
{
  const Color red{1, 0, 0, 1};
  const Color green{0, 1, 0, 1};
  const Color white{1, 1, 1, 1};
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

float unitBalance(float x);

void fillMapCells(std::vector<std::vector<int>>& map, Vec2Int pos, int size, int padding);
void fillDamageMap(std::vector<std::vector<int>>& map, Vec2Int pos, int radius, int attack);

std::vector<Entity> findFreePosOnBuildCellMap(std::vector<std::vector<int>>& map, int size);

bool isAvailable(std::vector<std::vector<int>>& map, Vec2Int pos, int size);

#endif