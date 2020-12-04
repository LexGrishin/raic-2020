#ifndef _UTILS_HPP_
#define _UTILS_HPP_

#include <iostream>
#include <string>

#include "model/Model.hpp"

namespace colors 
{
  const Color red{1, 0, 0, 1};
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

float unitBalance(float x);

void fillMapCells(std::vector<std::vector<int>>& map, Vec2Int pos, int size, int padding);

bool isAvailable(std::vector<std::vector<int>>& map, Vec2Int pos, int size);

#endif