#pragma once
#include <Components.hpp>

namespace dev
{

inline void DrawComponentUI(NameComponent& component, entt::entity entity)
{
    ImGui::InputText("Name", &component.name);
}

inline void DrawComponentUI(TransformComponent& component, entt::entity entity)
{
    ImGui::DragFloat3("Position", &component.position.x, 0.05f);
    ImGui::DragFloat3("Rotation", &component.rotation.x, 0.05f);
    ImGui::DragFloat3("Scale", &component.scale.x, 0.01f);

    if(ImGui::Button("Reset Scale"))
        component.scale = glm::vec3(1.0f);
}

inline void DrawComponentUI(MeshRendererComponent& component, entt::entity entity)
{
    float regionWidth = ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x;
    int cols = std::max(1, (int)(regionWidth / 128.0f));

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

    ImGui::Columns(cols, nullptr, false);

    for(size_t i = 0; i < component.materials.size(); i++)
    {
        ImGui::PushID(i);

        if(component.materials[i]->albedo.type == dev::MaterialAsset::Property::Type::Texture)
            ImGui::Image(component.materials[i]->albedo.texture->nativeHandle, ImVec2(128.0f, 128.0f));
        else
        {
            ImVec4 color = ImVec4(
                component.materials[i]->albedo.value.x,
                component.materials[i]->albedo.value.y,
                component.materials[i]->albedo.value.z,
                component.materials[i]->albedo.value.w
            );
            
            ImGui::PushStyleColor(ImGuiCol_Button, color);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
            
            ImGui::Button("##Asset", ImVec2(128.0f, 128.0f));
            
            ImGui::PopStyleColor(3); 
        }

        if(ImGui::BeginDragDropTarget())
        {
            auto payload = ImGui::AcceptDragDropPayload("MATERIAL");

            if(payload)
                component.materials[i] = *(MaterialAssetPtr*)payload->Data;

            ImGui::EndDragDropTarget();
        }

        if(ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            dev::MaterialAssetPtr* payload = &component.materials[i];
            
            ImGui::SetDragDropPayload("MATERIAL", payload, 8);    
            ImGui::Image(component.materials[i]->albedo.texture->nativeHandle, ImVec2(64,64));
            ImGui::Text("Material");

            ImGui::EndDragDropSource();
        }

        ImGui::NextColumn();

        ImGui::PopID();
    }

    ImGui::NextColumn();

    ImGui::Columns(1);

    ImGui::PopStyleVar();

    ImGui::Separator();

    if(ImGui::Button("Add Material"))
        component.materials.push_back(component.materials.back());
}

inline void DrawComponentUI(CameraComponent& component, entt::entity entity)
{
    if(ImGui::DragFloat("FOV", &component.camera.fov, 0.05f, 1.0f, 179.0f))
        component.camera.SetPerspective();

    if(ImGui::DragFloat("Near", &component.camera.near, 0.05f, 0.01f, 10.0f))
        component.camera.SetPerspective();

    if(ImGui::DragFloat("Far", &component.camera.far, 0.05f, 0.01f, 5000.0f))
        component.camera.SetPerspective();
}

inline void DrawComponentUI(LightComponent& component, entt::entity entity)
{
    ImGui::ColorEdit3("Color", &component.color.x);
    ImGui::DragFloat("Intensity", &component.intensity, 0.05f, 0.0f, 100.0f);
    ImGui::DragFloat("Cutoff", &component.cutoff, 0.05f, 0.0f, 360.0f);
    ImGui::DragFloat("Outer Cutoff", &component.outerCutoff, 0.05f, 0.0f, 360.0f);
    
    if(ImGui::Checkbox("Shadow map", &component.shadowMap))
        if(!component.renderTarget)
            component.SetupShadowMap(component.resolution);
    
    static const uint32_t min = 128, max = 8192;

    ImGui::DragScalarN("Resolution", ImGuiDataType_U32, &component.resolution.width, 2, 1.0f, &min, &max);
    ImGui::DragFloat("Bias", &component.bias, 0.0001f, 0.0f, 1.0f);

    if(ImGui::Button("Setup Shadow Map"))
        component.SetupShadowMap(component.resolution);
}

inline void DrawComponentUI(TonemapComponent& component, entt::entity entity)
{
    static const std::vector<const char*> algorithms =
    {
        "ACES",
        "ACES Film",
        "Reinhard",
        "Uncharted 2",
        "Filmic",
        "Lottes"
    };

    ImGui::DragFloat("Exposure", &component.exposure, 0.05f, 0.0f, 100.0f);
    ImGui::Combo("Algorithm", &component.algorithm, algorithms.data(), algorithms.size());

    ImGui::Separator();

    ImGui::ColorEdit3("Color Grading", &component.colorGrading.x);
    ImGui::DragFloat("Color Grading Intensity", &component.colorGradingIntensity, 0.01f, 0.0f, 1.0f);
    ImGui::DragFloat("Vignette Intensity", &component.vignetteIntensity, 0.01f, 0.0f, 1.0f);
    ImGui::DragFloat("Vignette Roundness", &component.vignetteRoundness, 0.01f, 0.0f, 1.0f);
    ImGui::DragFloat("Film Grain", &component.filmGrain, 0.01f, 0.0f, 10.0f);
    ImGui::DragFloat("Contrast", &component.contrast, 0.01f, 0.0f, 5.0f);
    ImGui::DragFloat("Saturation", &component.saturation, 0.01f, -5.0f, 5.0f);
    ImGui::DragFloat("Brightness", &component.brightness, 0.01f, 0.0f, 5.0f);
}

inline void DrawComponentUI(BloomComponent& component, entt::entity entity)
{
    ImGui::DragFloat("Threshold", &component.threshold, 0.01f, 0.0f, 10.0f);
    ImGui::DragFloat("Strength", &component.strength, 0.01f, 0.0f, 10.0f);
    ImGui::DragFloat("Resolution Scale##Bloom", &component.resolutionScale, 0.1f, 1.0f, 10.0f);

    ImGui::Separator();

    if(ImGui::Button("Update##Bloom"))
        component.SetupPostProcessing();
}

inline void DrawComponentUI(GTAOComponent& component, entt::entity entity)
{
    ImGui::DragFloat("Resolution Scale##GTAO", &component.resolutionScale, 0.1f, 1.0f, 10.0f);
    
    ImGui::DragInt("Samples", &component.samples, 1, 1, 1024);
    ImGui::DragFloat("Limit", &component.limit, 0.1f, 0.0f, 1000.0f);
    ImGui::DragFloat("Radius", &component.radius, 0.1f, 0.0f, 100.0f);
    ImGui::DragFloat("Falloff", &component.falloff, 0.1f, 0.0f, 100.0f);
    ImGui::DragFloat("Thickness Mix", &component.thicknessMix, 0.01f, 0.0f, 1.0f);
    ImGui::DragFloat("Max Stride", &component.maxStride, 0.1f, 0.0f, 100.0f);

    ImGui::Separator();

    if(ImGui::Button("Update##GTAO"))
        component.SetupPostProcessing();
}

inline void DrawComponentUI(SSRComponent& component, entt::entity entity)
{
    ImGui::DragFloat("Resolution Scale##SSR", &component.resolutionScale, 0.1f, 1.0f, 10.0f);
    ImGui::DragInt("Max Steps", &component.maxSteps, 1, 1, 10000);
    ImGui::DragInt("Max Binary Search Steps", &component.maxBinarySearchSteps, 1, 0, 1000);
    ImGui::DragFloat("Ray Step", &component.rayStep, 0.001f, 0.0f, 1.0f);

    ImGui::Separator();

    if(ImGui::Button("Update##SSR"))
        component.SetupPostProcessing();
}

inline void DrawComponentUI(ProceduralSkyComponent& component, entt::entity entity)
{
    ImGui::DragFloat("Time", &component.time, 0.1f, 0.0f, 1000.0f);
    ImGui::DragFloat("Cirrus", &component.cirrus, 0.001f, 0.0f, 1.0f);
    ImGui::DragFloat("Cumulus", &component.cumulus, 0.001f, 0.0f, 1.0f);
    
    static const uint32_t min = 128, max = 8192;

    if(ImGui::DragScalar("Resolution", ImGuiDataType_U32, &component.resolution.width, 8.0f, &min, &max))
        component.resolution.height = component.resolution.width;

    if(ImGui::Button("Convert"))
        Multithreading::Get().AddJob({ {}, [&]() { component.Build(); } });
}

inline void DrawComponentUI(HDRISkyComponent& component, entt::entity entity)
{
    ImGui::Text("Environment Map:");
    ImGui::Image(component.environmentMap->nativeHandle, ImVec2(128.0f, 128.0f));

    if(ImGui::BeginDragDropTarget())
    {
        auto payload = ImGui::AcceptDragDropPayload("TEXTURE");
        
        if(payload)
        {
            component.environmentMap = *(TextureAssetPtr*)payload->Data;

            Multithreading::Get().AddJob({ {}, [&]() { component.Build(); } });
        }

        ImGui::EndDragDropTarget();
    }

    static const uint32_t min = 128, max = 8192;

    if(ImGui::DragScalar("Resolution", ImGuiDataType_U32, &component.resolution.width, 8.0f, &min, &max))
        component.resolution.height = component.resolution.width;

    if(ImGui::Button("Convert"))
        Multithreading::Get().AddJob({ {}, [&]() { component.Build(); } });
}

inline void DrawComponentUI(RigidBodyComponent& component, entt::entity entity)
{
    static glm::vec3 scale(1.0);

    int motionType = (int)component.body->GetMotionType();

    static const std::vector<const char*> motionTypes =
    {
        "Static",
        "Kinematic",
        "Dynamic"
    };

    static const std::vector<const char*> shapeTypes =
    {
        "Empty",
        "Box",
        "Sphere",
        "Capsule",
        "Mesh"
    };

    static const std::unordered_map<JPH::EShapeSubType, size_t> shapeTypesToIndex =
    {
        { JPH::EShapeSubType::Empty, 0 },
        { JPH::EShapeSubType::Box, 1 },
        { JPH::EShapeSubType::Sphere, 2 },
        { JPH::EShapeSubType::Capsule, 3 },
        { JPH::EShapeSubType::Mesh, 4 }
    };

    static int shapeType = shapeTypesToIndex.at(component.body->GetShape()->GetSubType());

    if(ImGui::Combo("Motion type", &motionType, motionTypes.data(), motionTypes.size()))
        PhysicsManager::Get().GetBodyInterface().SetMotionType(
            component.body->GetID(),
            (JPH::EMotionType)motionType,
            JPH::EActivation::Activate
        );

    ImGui::Separator();

    if(ImGui::CollapsingHeader("Shape"))
    {
        ImGui::Indent();

        static std::shared_ptr<JPH::ShapeSettings> settings;

        if(ImGui::Combo("Shape type", &shapeType, shapeTypes.data(), shapeTypes.size()))
            settings.reset();

        switch(shapeType)
        {
            case 0:
            {
                static glm::vec3 centerOfMass(0.0f);

                if(!settings)
                    settings = std::make_shared<JPH::EmptyShapeSettings>();

                ImGui::DragFloat3("Center of mass", &centerOfMass.x, 0.05f);

                std::static_pointer_cast<JPH::EmptyShapeSettings>(settings)->
                    mCenterOfMass = { centerOfMass.x, centerOfMass.y, centerOfMass.z };

                break;
            }

            case 1:
            {
                static glm::vec3 halfExtent(1.0f);

                if(!settings)
                    settings = std::make_shared<JPH::BoxShapeSettings>();

                ImGui::DragFloat3("Half Extent", &halfExtent.x, 0.05f);

                std::static_pointer_cast<JPH::BoxShapeSettings>(settings)->
                    mHalfExtent = { halfExtent.x, halfExtent.y, halfExtent.z };

                break;
            }

            case 2:
            {
                static float radius(1.0f);

                if(!settings)
                    settings = std::make_shared<JPH::SphereShapeSettings>();

                ImGui::DragFloat("Radius", &radius, 0.05f);

                std::static_pointer_cast<JPH::SphereShapeSettings>(settings)->
                    mRadius = radius;

                break;
            }

            case 3:
            {
                static float radius(1.0f);
                static float halfHeight(1.0f);

                if(!settings)
                    settings = std::make_shared<JPH::CapsuleShapeSettings>();

                ImGui::DragFloat("Radius", &radius, 0.05f);
                ImGui::DragFloat("Half Height", &halfHeight, 0.05f);

                std::static_pointer_cast<JPH::CapsuleShapeSettings>(settings)->
                    mRadius = radius;
                std::static_pointer_cast<JPH::CapsuleShapeSettings>(settings)->
                    mHalfHeightOfCylinder = halfHeight;

                break;
            }

            case 4:
            {
                static ModelAssetPtr model;

                ImGui::Button(model ? "Model##Model" : "(Empty)##Model", ImVec2(128.0f, 128.0f));

                if(ImGui::BeginDragDropTarget())
                {
                    auto payload = ImGui::AcceptDragDropPayload("MODEL");

                    if(payload)
                    {
                        model = *(dev::ModelAssetPtr*)payload->Data;

                        JPH::TriangleList list;

                        for(auto& mesh : model->meshes)
                        {
                            auto vertices = mesh->GetVertices();
                            auto indices = mesh->GetIndices();

                            list.reserve(list.size() + indices.size() / 3);

                            for(size_t i = 0; i < indices.size(); i += 3)
                            {
                                auto pos0 = vertices[indices[i]].position;
                                auto pos1 = vertices[indices[i + 1]].position;
                                auto pos2 = vertices[indices[i + 2]].position;

                                list.emplace_back(
                                    JPH::Float3(pos0.x, pos0.y, pos0.z),
                                    JPH::Float3(pos1.x, pos1.y, pos1.z),
                                    JPH::Float3(pos2.x, pos2.y, pos2.z)
                                );
                            }
                        }

                        if(!settings)
                            settings = std::make_shared<JPH::MeshShapeSettings>(list);
                    }

                    ImGui::EndDragDropTarget();
                }

                break;
            }
        }

        ImGui::Separator();

        if(ImGui::Button("Update shape") && settings)
        {
            auto body = component.body;
            auto bodyId = body->GetID();

            PhysicsManager::Get().GetBodyInterface().SetShape(
                bodyId,
                settings->Create().Get(),
                false,
                JPH::EActivation::Activate
            );
        }

        ImGui::Unindent();
    }

    ImGui::DragFloat3("Scale##Shape", &scale.x, 0.05f);

    if(ImGui::Button("Apply scale"))
    {
        if(scale != glm::vec3(1.0))
            PhysicsManager::Get().GetBodyInterface().SetShape(
                component.body->GetID(),
                component.body->GetShape()->ScaleShape({ scale.x, scale.y, scale.z }).Get(),
                false,
                JPH::EActivation::Activate
            );

        scale = glm::vec3(1.0);
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
        ImGui::PushID((entt::id_type)entity);

        if constexpr (HasComponentUI<Component>::value)
            if(ImGui::CollapsingHeader(component->componentName.data()))
            {
                DrawComponentUI(*component, entity);
                
                ImGui::PushID(component->componentName.data());

                if(ImGui::Button("Remove"))
                    registry.remove<Component>(entity);

                ImGui::PopID();
            }

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
