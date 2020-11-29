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

namespace positions
{
    const auto debug_info_pos = std::make_shared<Vec2Float>(80, 80);
}

#endif