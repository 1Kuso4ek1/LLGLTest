#pragma once
#include <JoltInclude.hpp>

namespace dev
{

namespace Layers
{
    static constexpr JPH::ObjectLayer nonMoving = 0;
    static constexpr JPH::ObjectLayer moving = 1;
    static constexpr JPH::ObjectLayer numLayers = 2;
};

namespace BroadPhaseLayers
{
	static constexpr JPH::BroadPhaseLayer nonMoving(0);
	static constexpr JPH::BroadPhaseLayer moving(1);
	static constexpr uint numLayers(2);
};

}
