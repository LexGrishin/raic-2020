#include "utils.hpp"
#include <math.h>

int distance(Entity& e1, Entity& e2, std::unordered_map<EntityType, EntityProperties>& properties)
{
    int dx, dy;
    int e1_xmin = e1.position.x;
    int e1_xmax = e1.position.x + properties.at(e1.entityType).size - 1;
    int e1_ymin = e1.position.y;
    int e1_ymax = e1.position.y + properties.at(e1.entityType).size - 1;
    int e2_xmin = e2.position.x;
    int e2_xmax = e2.position.x + properties.at(e2.entityType).size - 1;
    int e2_ymin = e2.position.y;
    int e2_ymax = e2.position.y + properties.at(e2.entityType).size - 1;

    if ((e1_xmin >= e2_xmin && e1_xmin <= e2_xmax)
     || (e1_xmax >= e2_xmin && e1_xmax <= e2_xmax)
     || (e2_xmin >= e1_xmin && e2_xmin <= e1_xmax)
     || (e2_xmax >= e1_xmin && e2_xmax <= e1_xmax))
        dx = 0;
    else if (e1_xmin < e2_xmin)
        dx = e2_xmin - e1_xmin - properties.at(e1.entityType).size + 1;
    else
        dx = e1_xmin - e2_xmin - properties.at(e2.entityType).size + 1;

    if ((e1_ymin >= e2_ymin && e1_ymin <= e2_ymax)
     || (e1_ymax >= e2_ymin && e1_ymax <= e2_ymax)
     || (e2_ymin >= e1_ymin && e2_ymin <= e1_ymax)
     || (e2_ymax >= e1_ymin && e2_ymax <= e1_ymax))
        dy = 0;
    else if (e1_ymin < e2_ymin)
        dy = e2_ymin - e1_ymin - properties.at(e1.entityType).size + 1;
    else
        dy = e1_ymin - e2_ymin - properties.at(e2.entityType).size + 1;
    return dx + dy - 1;
}
