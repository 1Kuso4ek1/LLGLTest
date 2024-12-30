#pragma once
#include <TextureManager.hpp>
#include <Mesh.hpp>
#include <ImGuiManager.hpp>
#include <Camera.hpp>
#include <PostProcessing.hpp>

#include <entt/entt.hpp>

namespace dev
{

class Entity;

struct NameComponent
{
    std::string name;
};

struct TransformComponent
{
    glm::vec3 position = { 0.0f, 0.0f, 0.0f };
    glm::vec3 rotation = { 0.0f, 0.0f, 0.0f };
    glm::vec3 scale =    { 1.0f, 1.0f, 1.0f };

    glm::mat4 GetTransform() const;
};

struct MeshComponent
{
    std::vector<std::shared_ptr<Mesh>> meshes;
};

struct MeshRendererComponent
{
    std::vector<std::shared_ptr<TextureHandle>> materials;
};

struct PipelineComponent
{
    LLGL::PipelineState* pipeline;
};

struct CameraComponent
{
    Camera camera;
};

struct LightComponent
{
    glm::vec3 color = { 1.0f, 1.0f, 1.0f };

    float intensity = 1.0f, cutoff = 0.0f, outerCutoff = 0.0f;
};

struct ScriptComponent
{
    std::function<void()> start;
    std::function<void(Entity self, float)> update;
};

struct ACESTonemappingComponent
{
    ACESTonemappingComponent();

    float exposure = 1.0f;

    std::shared_ptr<PostProcessing> postProcessing;

    std::function<void(LLGL::CommandBuffer*)> setUniforms;
};

struct ProceduralSkyComponent
{
    ProceduralSkyComponent();

    float time = 100.0f, cirrus = 0.0f, cumulus = 0.0f;

    // Maybe it will be better to pass pipeline as a separate component?
    LLGL::PipelineState* pipeline;

    std::function<void(LLGL::CommandBuffer*)> setUniforms;
};

struct HDRISkyComponent
{
    HDRISkyComponent() = default;
};

using Drawable = std::tuple<TransformComponent, MeshComponent, MeshRendererComponent, PipelineComponent>;

inline void DrawComponentUI(NameComponent& component, entt::entity entity)
{
    if(ImGui::CollapsingHeader("NameComponent"))
        ImGui::InputText("Name", &component.name);
}

inline void DrawComponentUI(TransformComponent& component, entt::entity entity)
{
    if(ImGui::CollapsingHeader("TransformComponent"))
    {
        ImGui::DragFloat3("Position", &component.position.x, 0.05f);
        ImGui::DragFloat3("Rotation", &component.rotation.x, 0.05f);
        ImGui::DragFloat3("Scale", &component.scale.x, 0.01f);

        if(ImGui::Button("Reset Scale"))
            component.scale = glm::vec3(1.0f);
    }
}

inline void DrawComponentUI(CameraComponent& component, entt::entity entity)
{
    if(ImGui::CollapsingHeader("CameraComponent"))
    {
        if(ImGui::DragFloat("FOV", &component.camera.fov, 0.05f, 1.0f, 179.0f))
            component.camera.SetPerspective();

        if(ImGui::DragFloat("Near", &component.camera.near, 0.05f, 0.01f, 10.0f))
            component.camera.SetPerspective();

        if(ImGui::DragFloat("Far", &component.camera.far, 0.05f, 0.01f, 5000.0f))
            component.camera.SetPerspective();
    }
}

inline void DrawComponentUI(LightComponent& component, entt::entity entity)
{
    if(ImGui::CollapsingHeader("LightComponent"))
    {
        ImGui::ColorEdit3("Color", &component.color.x);
        ImGui::DragFloat("Intensity", &component.intensity, 0.05f, 0.0f, 100.0f);
        ImGui::DragFloat("Cutoff", &component.cutoff, 0.05f, 0.0f, 360.0f);
        ImGui::DragFloat("Outer Cutoff", &component.outerCutoff, 0.05f, 0.0f, 360.0f);
    }
}

inline void DrawComponentUI(ACESTonemappingComponent& component, entt::entity entity)
{
    if(ImGui::CollapsingHeader("ACESTonemappingComponent"))
    {
        ImGui::DragFloat("Exposure", &component.exposure, 0.05f, 0.0f, 100.0f);
    }
}

inline void DrawComponentUI(ProceduralSkyComponent& component, entt::entity entity)
{
    if(ImGui::CollapsingHeader("ProceduralSkyComponent"))
    {
        ImGui::DragFloat("Time", &component.time, 0.1f, 0.0f, 1000.0f);
        ImGui::DragFloat("Cirrus", &component.cirrus, 0.001f, 0.0f, 1.0f);
        ImGui::DragFloat("Cumulus", &component.cumulus, 0.001f, 0.0f, 1.0f);
    }
}

// Jesus Christ what is that
template<class T>
struct HasComponentUI
{
    template<class U> static char Test(void(*)(U&, entt::entity));
    template<class U> static int Test(...);

    static const bool value = sizeof(Test<T>(nullptr)) == sizeof(char);
};

// This is just magic
template <typename... Components>
void DrawEntityUI(entt::registry& registry, entt::entity entity)
{
    if(!registry.valid(entity))
        return;

    auto draw = [&]<typename Component>(Component* component)
    {
        ImGui::PushID((uint64_t)entity);

        if constexpr (HasComponentUI<Component>::value)
            DrawComponentUI(*component, entity);

        ImGui::PopID();
    };

    ([&]<typename Component>()
    {
        Component* component = registry.try_get<Component>(entity);
        if(component)
            draw(component);
    }.template operator()<Components>(), ...); 
}

}
