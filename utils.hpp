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

int distance(Entity e1, Entity e2, std::unordered_map<EntityType, EntityProperties> properties);

#endif