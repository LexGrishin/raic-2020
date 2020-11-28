#include "MyStrategy.hpp"
#include <exception>

MyStrategy::MyStrategy() {}

Action MyStrategy::getAction(const PlayerView& playerView, DebugInterface* debugInterface)
{
    Color red{255, 0, 0, 255};
    auto x = playerView.mapSize / 10;
    auto point = std::make_shared<Vec2Float>(x, x);
    auto debugText = std::make_shared<DebugData::PlacedText>(ColoredVertex(point, Vec2Float(0, 0), red), "hellow world!", 0, 200);
    debugInterface->send(DebugCommand::Clear());
    debugInterface->send(DebugCommand::Add(debugText));
    debugInterface->getState();
    return Action(std::unordered_map<int, EntityAction>());
}

void MyStrategy::debugUpdate(const PlayerView& playerView, DebugInterface& debugInterface)
{
    debugInterface.send(DebugCommand::Clear());
    debugInterface.getState();
}