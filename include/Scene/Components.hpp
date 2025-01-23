#pragma once
#include <Mesh.hpp>
#include <ImGuiManager.hpp>
#include <Camera.hpp>
#include <PostProcessing.hpp>
#include <AssetManager.hpp>
#include <TextureAsset.hpp>
#include <MaterialAsset.hpp>
#include <ModelAsset.hpp>

#include <entt/entt.hpp>

namespace dev
{

class Entity;

struct ComponentBase
{
    ComponentBase(const std::string_view& componentName) : componentName(componentName) {}
    virtual ~ComponentBase() = default;

    std::string_view componentName;
};

struct NameComponent : public ComponentBase
{
    NameComponent() : ComponentBase("NameComponent") {}

    std::string name;
};

struct TransformComponent : public ComponentBase
{
    TransformComponent() : ComponentBase("TransformComponent") {}

    glm::vec3 position = { 0.0f, 0.0f, 0.0f };
    glm::vec3 rotation = { 0.0f, 0.0f, 0.0f };
    glm::vec3 scale =    { 1.0f, 1.0f, 1.0f };

    void SetTransform(const glm::mat4& transform);
    
    glm::mat4 GetTransform() const;
};

struct MeshComponent : public ComponentBase
{
    MeshComponent() : ComponentBase("MeshComponent") {}

    ModelAssetPtr model;
};

struct MeshRendererComponent : public ComponentBase
{
    MeshRendererComponent() : ComponentBase("MeshRendererComponent")
    {
        materials.push_back(AssetManager::Get().Load<MaterialAsset>("default", true));
    }

    std::vector<MaterialAssetPtr> materials;
};

struct PipelineComponent : public ComponentBase
{
    PipelineComponent() : ComponentBase("PipelineComponent") {}

    LLGL::PipelineState* pipeline{};
};

struct CameraComponent : public ComponentBase
{
    CameraComponent() : ComponentBase("CameraComponent") {}

    Camera camera;
};

struct LightComponent : public ComponentBase
{
public:
    LightComponent();

    void SetupShadowMap(const LLGL::Extent2D& resolution);

    glm::vec3 color = { 1.0f, 1.0f, 1.0f };

    float intensity = 1.0f;
    float cutoff = 0.0f, outerCutoff = 0.0f;
    float bias = 0.00001f;

    bool shadowMap = false;

    LLGL::Extent2D resolution;

    LLGL::Texture* depth{};
    LLGL::RenderTarget* renderTarget{};
    LLGL::PipelineState* shadowMapPipeline{};

    // Make it a single light space matrix
    glm::mat4 projection;

private:
    void CreateDepth(const LLGL::Extent2D& resolution);
    void CreateRenderTarget(const LLGL::Extent2D& resolution);
    void CreatePipeline();
};

struct ScriptComponent : public ComponentBase
{
    ScriptComponent() : ComponentBase("ScriptComponent") {}

    std::function<void()> start;
    std::function<void(Entity self, float)> update;
};

struct ACESTonemappingComponent : public ComponentBase
{
    ACESTonemappingComponent(
        const LLGL::Extent2D& resolution = Renderer::Get().GetSwapChain()->GetResolution(),
        bool registerEvent = true
    );

    float exposure = 1.0f;

    std::shared_ptr<PostProcessing> postProcessing;

    std::function<void(LLGL::CommandBuffer*)> setUniforms;
};

struct ProceduralSkyComponent : public ComponentBase
{
    ProceduralSkyComponent();

    float time = 40.0f, cirrus = 0.0f, cumulus = 0.0f;

    // Maybe it will be better to pass pipeline as a separate component?
    LLGL::PipelineState* pipeline{};

    std::function<void(LLGL::CommandBuffer*)> setUniforms;
};

struct HDRISkyComponent : public ComponentBase, EventListener
{
public:
    HDRISkyComponent(dev::TextureAssetPtr hdri, const LLGL::Extent2D& resolution);

    void Build();

    void OnEvent(Event& event) override;

    void SetResolution(const LLGL::Extent2D& resolution);

    TextureAssetPtr environmentMap;

    LLGL::PipelineState* pipelineSky{};

    // Make it something like PBRStack component idk
    LLGL::Texture* cubeMap{};
    LLGL::Texture* irradiance{};
    LLGL::Texture* prefiltered{};
    LLGL::Texture* brdf{};
    ////////////////////////////////////////////////

    LLGL::Extent2D resolution;

private:
    void RenderCubeMap(
        const std::unordered_map<uint32_t, LLGL::Resource*>& resources,
        LLGL::Texture* cubeMap,
        LLGL::PipelineState* pipeline
    );

    void RenderCubeMapMips(
        const std::unordered_map<uint32_t, LLGL::Resource*>& resources,
        LLGL::Texture* cubeMap,
        LLGL::PipelineState* pipeline
    );

    void RenderBRDF();

    void SetupConvertPipeline();
    void SetupIrradiancePipeline();
    void SetupPrefilteredPipeline();
    void SetupBRDFPipeline();
    void SetupSkyPipeline();

    void CreateCubemaps(const LLGL::Extent2D& resolution);
    void CreateRenderTargets(const LLGL::Extent2D& resolution, LLGL::Texture* cubeMap, int mipLevel = 0);

    void CreateBRDFTexture(const LLGL::Extent2D& resolution);
    void CreateBRDFRenderTarget(const LLGL::Extent2D& resolution);

    void ReleaseCubeMaps();
    void ReleaseRenderTargets();

private:
    LLGL::PipelineState* pipelineConvert{};
    LLGL::PipelineState* pipelineIrradiance{};
    LLGL::PipelineState* pipelinePrefiltered{};
    LLGL::PipelineState* pipelineBRDF{};

    LLGL::RenderTarget* brdfRenderTarget{};

    std::array<LLGL::RenderTarget*, 6> renderTargets;

    std::array<glm::mat4, 6> views =
    {
        glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f))
    };

    glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 1000.0f);
};

}
