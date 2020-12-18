#include "utils.hpp"

WayPoint::WayPoint(Vec2Int position, float g, float h, std::shared_ptr<WayPoint> parent)
{
    this->g = g;
    this->h = h;
    this->f = g + h;
    this->parent = parent;
    this->position = position;
}

bool operator< (const WayPoint& p1, const WayPoint& p2)
{
    return (p1.f > p2.f);
}

int WayPoint::id()
{
    int mapSize = 80;
    return (position.x + position.y * mapSize);
}

int distance(Vec2Int& a, Vec2Int& b)
{
    int dx = abs(a.x - b.x);
    int dy = abs(a.y - b.y);
    return dx + dy;
}

float unitBalance(float x)
{
    float balance = 0;
    if (x > 470)
        balance = 0.4;
    else
        balance = (-(x - 400.)/(200. + abs(x - 400.)) + 0.8)/2. + 0.1;
    return balance;
}

void fillMapCells(std::vector<std::vector<int>>& map, Vec2Int pos, int value, int size, int padding)
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
        {
            map[i][j] = value;
        }
            
}
void fillDamageMap(std::vector<std::vector<int>>& map, Vec2Int pos, int radius, int damage)
{
    int mapSize = map[0].size();
    int x_min = 0;
    int x_max = 0;
    if (pos.x - radius <= 0) x_min = 0; else x_min = pos.x - radius;
    if (pos.x + radius >= mapSize) x_max = mapSize; else x_max = pos.x + radius + 1;

    for (int x = x_min; x < x_max; x++)
    {
        int y_min = 0;
        int y_max = 0;
        int dy = radius - abs(x - pos.x);
        if (pos.y - dy <= 0) y_min = 0; else y_min = pos.y - dy;
        if (pos.y + dy >= mapSize) y_max = mapSize; else y_max = pos.y + dy + 1;
        for (int y = y_min; y < y_max; y++)
            map[x][y] += damage;
    }
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

bool isPassable(int var)
{
    bool result = false;
    if (var == -1 || var == EntityType::RESOURCE)
    {
        result = true;
    }
    return result;
}


bool exists(std::vector<std::vector<int>>& map, Vec2Int& pos)
{
    int mapSize = map[0].size();
    bool result = true;
    if (pos.x < 0 || pos.y < 0 || pos.x >= mapSize || pos.y >= mapSize) result = false;
    return result;

}

float cross(Vec2Int& start, Vec2Int& current, Vec2Int& target)
{
    float dx1 = current.x - target.x;
    float dy1 = current.y - target.y;
    float dx2 = start.x - target.x;
    float dy2 = start.y - target.y;
    float result = abs(dx1*dy2 - dx2*dy1) * 0.0001;
    return result;
}

Vec2Int getRetreatPos(Vec2Int pos, std::vector<std::vector<int>>& mapDamage, std::vector<std::vector<int>>& mapOccupied)
{
    int mapSize = mapDamage[0].size();
    int topDamage = 100000;
    int leftDamage = 100000;
    int rightDamage = 100000;
    int botDamage = 100000;
    int stayDamage = 0;
    int radius = 5;
    Vec2Int posTop{pos.x, pos.y + 1};
    Vec2Int posBot{pos.x, pos.y - 1};
    Vec2Int posRight{pos.x + 1, pos.y};
    Vec2Int posLeft{pos.x - 1, pos.y};
    stayDamage = countDamageSum(pos, radius, mapDamage);
    if (posTop.y < mapSize)
        if (mapOccupied[posTop.x][posTop.y] == -1)
            topDamage = countDamageSum(posTop, radius, mapDamage);
    if (posBot.y >= 0)
        if (mapOccupied[posBot.x][posBot.y] == -1)
            botDamage = countDamageSum(posBot, radius, mapDamage);
    if (posRight.x < mapSize)
        if (mapOccupied[posRight.x][posRight.y] == -1)
            rightDamage = countDamageSum(posRight, radius, mapDamage);
    if (posLeft.x >= 0)
        if (mapOccupied[posLeft.x][posLeft.y] == -1)
            leftDamage = countDamageSum(posLeft, radius, mapDamage);
    int minDamage = stayDamage;
    Vec2Int minPos = pos;
    if (topDamage < minDamage)
    {
        minDamage = topDamage;
        minPos = posTop;
    }
    if (botDamage < minDamage)
    {
        minDamage = botDamage;
        minPos = posBot;
    }
    if (rightDamage < minDamage)
    {
        minDamage = rightDamage;
        minPos = posRight;
    }
    if (leftDamage < minDamage)
    {
        minDamage = leftDamage;
        minPos = posLeft;
    }
    return minPos;
}

int countUnitsInRadius(Vec2Int pos, int radius, std::vector<std::vector<int>>& map)
{
    int counter = 0;
    int mapSize = map[0].size();
    int x_min = 0;
    int x_max = 0;
    if (pos.x - radius <= 0) x_min = 0; else x_min = pos.x - radius;
    if (pos.x + radius >= mapSize) x_max = mapSize; else x_max = pos.x + radius + 1;

    for (int x = x_min; x < x_max; x++)
    {
        int y_min = 0;
        int y_max = 0;
        int dy = radius - abs(x - pos.x);
        if (pos.y - dy <= 0) y_min = 0; else y_min = pos.y - dy;
        if (pos.y + dy >= mapSize) y_max = mapSize; else y_max = pos.y + dy + 1;
        for (int y = y_min; y < y_max; y++)
            if (map[x][y] != -1)
                counter += 1;
    }
    return counter;
}

int countDamageSum(Vec2Int pos, int radius, std::vector<std::vector<int>>& map)
{
    int counter = 0;
    int mapSize = map[0].size();
    int x_min = 0;
    int x_max = 0;
    if (pos.x - radius <= 0) x_min = 0; else x_min = pos.x - radius;
    if (pos.x + radius >= mapSize) x_max = mapSize; else x_max = pos.x + radius + 1;

    for (int x = x_min; x < x_max; x++)
    {
        int y_min = 0;
        int y_max = 0;
        int dy = radius - abs(x - pos.x);
        if (pos.y - dy <= 0) y_min = 0; else y_min = pos.y - dy;
        if (pos.y + dy >= mapSize) y_max = mapSize; else y_max = pos.y + dy + 1;
        for (int y = y_min; y < y_max; y++)
            counter += map[x][y];
    }
    return counter;
}