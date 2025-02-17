#pragma once
#include <Singleton.hpp>

#include <LayerFilters.hpp>
#include <BroadPhaseLayer.hpp>
#include <CollisionListener.hpp>

namespace dev
{

class PhysicsManager : public Singleton<PhysicsManager>
{
public:
    ~PhysicsManager();

    void Init();
    void Update(float deltaTime);
    void DestroyBody(const JPH::BodyID& bodyId);

    JPH::Shape* CreateBoxShape(const JPH::BoxShapeSettings& settings);

    JPH::Body* CreateBody(const JPH::BodyCreationSettings& settings);

    JPH::PhysicsSystem& GetPhysicsSystem();
    JPH::BodyInterface& GetBodyInterface();

private:
    std::unique_ptr<CollisionListener> collisionListener;
    std::unique_ptr<JPH::JobSystemThreadPool> jobSystem;
    std::unique_ptr<JPH::TempAllocatorImpl> tempAllocator;

    JPH::PhysicsSystem physicsSystem;

    BroadPhaseLayerInterface broadPhaseLayer{};
	ObjectVsBroadPhaseLayerFilter objectVsBroadPhaseFilter{};
	ObjectLayerPairFilter objectLayerPairFilter{};
};

}
