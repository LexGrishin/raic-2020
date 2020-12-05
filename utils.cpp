#include "utils.hpp"
#include <math.h>

float unitBalance(float x)
{
    // float balance = (-(x - 200.)/(200. + abs(x - 200.)) + 0.8)/2. + 0.1;
    float balance = (-(x - 400.)/(200. + abs(x - 400.)) + 0.8)/2. + 0.1;
    return balance;
}

void fillMapCells(std::vector<std::vector<int>>& map, Vec2Int pos, int size, int padding)
{
    int map_size = map[0].size();
    int x_start, x_finish, y_start, y_finish;
    if (pos.x - padding < 0)
        x_start = pos.x;
    else
        x_start = pos.x - padding;

    if (pos.y - padding < 0)
        y_start = pos.y;
    else
        y_start = pos.y - padding;

    if (pos.x + size + padding > map_size)
        x_finish = pos.x + size;
    else
        x_finish = pos.x + size + padding;
    
    if (pos.y + size + padding > map_size)
        y_finish = pos.y + size;
    else
        y_finish = pos.y + size + padding;

    for (int i = x_start; i < x_finish; i++)
        for (int j = y_start; j < y_finish; j++)
            map[i][j] = 1;
}

std::vector<Entity> findFreePosOnBuildCellMap(std::vector<std::vector<int>>& map, int size)
{
    std::vector<Entity> positions;
    int mapSize = map[0].size();
    int limit = mapSize - size;
    int step = size + 1;
    for (int i = 0; i < limit; i+=step)
        for (int j = 0; j < limit; j+=step)
        {
            bool freeArea = true;
            for (int n = i; n < i + size; n++)
            {
                for (int m = j; m < j + size; m++)
                    if (map[n][m] != 0)
                    {
                        freeArea = false;
                        break;
                    }
                if (freeArea == false)
                    break;
            }
            if (freeArea == true)
            {
                Vec2Int pos{i, j};
                Entity position;
                position.position = pos;
                position.entityType = EntityType::HOUSE;
                positions.emplace_back(position);
            }                       
        }
    return positions;
}

bool isAvailable(std::vector<std::vector<int>>& map, Vec2Int pos, int size)
{
    int mapSize = map[0].size();
    int left = pos.x - 1;
    int right = pos.x + size;
    int top = pos.y + size;
    int bot = pos.y - 1;

    if (left >= 0)
        for (int i = pos.y; i < pos.y + size; i++)
            if (map[left][i] == 0)
                return true;
    if (right < mapSize)
        for (int i = pos.y; i < pos.y + size; i++)
            if (map[right][i] == 0)
                return true;
    if (top < mapSize)
        for (int i = pos.x; i < pos.x + size; i++)
            if (map[i][top] == 0)
                return true;
    if (bot >= 0)
        for (int i = pos.x; i < pos.x + size; i++)
            if (map[i][bot] == 0)
                return true;
    return false;
}
