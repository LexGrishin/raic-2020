#include "MyStrategy.hpp"
#include <exception>
#include <iostream>
#include <string>
#include <typeinfo>

MyStrategy::MyStrategy() {}

Action MyStrategy::getAction(const PlayerView playerView, DebugInterface* debugInterface)
{
    this->debugData.clear();
    this->debugData.emplace_back(new DebugData::PlacedText(ColoredVertex(positions::debug_info_pos, 
                                                                         Vec2Float(0, 0), colors::white), 
                                                                         "Map size: "+std::to_string(playerView.mapSize), 
                                                                         0, 20));
    this->debugData.emplace_back(new DebugData::PlacedText(ColoredVertex(positions::debug_info_pos, 
                                                                         Vec2Float(0, -20), colors::white), 
                                                                         "My ID: "+std::to_string(playerView.myId), 
                                                                         0, 20));

    std::unordered_map<int, EntityAction> orders{};

    for (Entity entity : playerView.entities)
    {
        if (entity.playerId == nullptr)
        {
            // std::cout << entity.entityType << std::endl;
        }
        // if (entity.entityType != EntityType::RESOURCE)
        // {
        //     std::cout << *entity.playerId << " ";
        // }
        // std::cout << *entity.playerId << " ";
        // if (entity.playerId == this->myId)
        // {
            // std::cout << "my unit" << std::endl;
            // switch (entity.entityType)
            // {
            // case EntityType::BUILDER_UNIT:
            //     std::cout << "builder unit (" << entity.position.x << ", " << entity.position.y << ")" << std::endl;
            //     break;
            
            // default:
            //     break;
            // }
        // }
    }
    // ColoredVertex A{std::make_shared<Vec2Float>(10, 10), {0, 0}, colors::red};
    // ColoredVertex B{std::make_shared<Vec2Float>(100, 10), {0, 0}, colors::red};
    // ColoredVertex C{std::make_shared<Vec2Float>(50, 100), {0, 0}, colors::red};
    // std::vector<ColoredVertex> triangle{A, B, C};
    // auto debugTriangle = std::make_shared<DebugData::Primitives>(triangle, PrimitiveType::TRIANGLES);

    // std::cout << "map size = " << playerView.mapSize << std::endl;
    // std::cout << "entities = " << playerView.entities.size() << std::endl;
    // std::cout << "float = " << sizeof(float) << std::endl;
    return Action(orders);
}

void MyStrategy::debugUpdate(const PlayerView& playerView, DebugInterface& debugInterface)
{
    debugInterface.send(DebugCommand::Clear());
    for (auto item : this->debugData)
    {
        debugInterface.send(DebugCommand::Add(item));
    }
    
    debugInterface.getState();
}

void MyStrategy::setGlobals(const PlayerView& playerView)
{
    if (this->myId == -1)
    {
        this->myId = playerView.myId;
    }
    this->isGlobalsSet = true;
}
