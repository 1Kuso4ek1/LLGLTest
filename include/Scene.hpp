#pragma once
#include <Components.hpp>
#include <Renderer.hpp>

#include <entt/entt.hpp>
#include <unordered_map>

namespace dev
{

class Entity;

class Scene
{
public:
    void Update();
    void Draw();

    Entity CreateEntity();

    entt::registry& GetRegistry();

private:
    entt::registry registry;

    uint64_t idCounter = 0;

    std::unordered_map<uint64_t, entt::entity> entities;

private:
    friend class Entity;
};

}