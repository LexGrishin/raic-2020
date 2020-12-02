#include "utils.hpp"
#include <math.h>

float unitBalance(float x)
{
    float balance = (-(x - 200.)/(200. + abs(x - 200.)) + 0.8)/2.;
    return balance;
}
