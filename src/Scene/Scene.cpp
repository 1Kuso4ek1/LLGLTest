#include <Scene.hpp>
#include <Entity.hpp>

namespace dev
{

Scene::Scene(std::shared_ptr<RendererBase> renderer)
    : renderer(renderer)
{
}

void Scene::SetRenderer(std::shared_ptr<RendererBase> renderer)
{
    this->renderer = renderer;
}

void Scene::Start()
{
    if(!lightsBuffer)
        SetupLightsBuffer();

    registry.view<ScriptComponent>().each([](auto& script)
    {
        if(script.start)
            script.start();
    });
}

void Scene::Update(float deltaTime)
{
    registry.view<ScriptComponent>().each([&](auto entity, auto& script)
    {
        if(script.update)
            script.update(Entity{ entity, this }, deltaTime);
    });
}

void Scene::Draw()
{
    SetupCamera();
    SetupLights();

    Renderer::Get().Begin();

    UpdateLightsBuffer();

    RenderMeshes();
    
    Renderer::Get().End();

    Renderer::Get().Submit();

    ApplyPostProcessing();
}

void Scene::RemoveEntity(const Entity& entity)
{
    registry.destroy(entity);
}

Entity Scene::CreateEntity()
{
    Entity entity{ registry.create(), this };

    entities[idCounter++] = entity;

    return entity;
}

entt::registry& Scene::GetRegistry()
{
    return registry;
}

void Scene::SetupLightsBuffer()
{
    static const uint64_t maxLights = 128;

    LLGL::BufferDescriptor lightBufferDesc = LLGL::ConstantBufferDesc(maxLights * sizeof(Light));
    
    lightsBuffer = Renderer::Get().CreateBuffer(lightBufferDesc);

    lights.reserve(maxLights);
}

void Scene::UpdateLightsBuffer()
{
    Renderer::Get().RenderPass(
        [&](auto commandBuffer)
        {
            commandBuffer->UpdateBuffer(*lightsBuffer, 0, lights.data(), lights.size() * sizeof(Light));
        }, {}, [](auto){}, nullptr
    );
}

void Scene::SetupCamera()
{
    auto cameraView = registry.view<TransformComponent, CameraComponent>();

    Camera* cam{};
    TransformComponent cameraTransform;

    for(auto entity : cameraView)
    {
        cam = &cameraView.get<CameraComponent>(entity).camera;
        cameraTransform = cameraView.get<TransformComponent>(entity);

        cameraPosition = cameraTransform.position;

        break;
    }

    if(cam)
    {
        auto delta = glm::quat(glm::radians(cameraTransform.rotation)) * glm::vec3(0.0f, 0.0f, -1.0f);
        
        Renderer::Get().GetMatrices()->GetView() = glm::lookAt(cameraTransform.position, cameraTransform.position + delta, glm::vec3(0.0f, 1.0f, 0.0f));
        Renderer::Get().GetMatrices()->GetProjection() = cam->GetProjectionMatrix();
    }
}

void Scene::SetupLights()
{
    lights.clear();

    auto lightsView = registry.view<LightComponent, TransformComponent>();

    for(auto entity : lightsView)
    {
        auto [light, transform] = lightsView.get<LightComponent, TransformComponent>(entity);

        lights.push_back(
            {
                transform.position,
                glm::quat(glm::radians(transform.rotation)) * glm::vec3(0.0f, 0.0f, -1.0f),
                light.color,
                light.intensity,
                glm::cos(glm::radians(light.cutoff)),
                glm::cos(glm::radians(light.outerCutoff))
            }
        );
    }
}

void Scene::RenderMeshes()
{
    auto view = registry.view<TransformComponent, MeshComponent, MeshRendererComponent, PipelineComponent>();

    for(auto entity : view)
    {
        auto [transform, mesh, meshRenderer, pipeline] = 
                view.get<TransformComponent, MeshComponent, MeshRendererComponent, PipelineComponent>(entity);

        Renderer::Get().GetMatrices()->PushMatrix();
        Renderer::Get().GetMatrices()->GetModel() = transform.GetTransform();

        MeshRenderPass(mesh, meshRenderer, pipeline, renderer->GetPrimaryRenderTarget());

        Renderer::Get().GetMatrices()->PopMatrix();
    }

    if(view.begin() == view.end())
        Renderer::Get().RenderPass(
            [](auto){}, {}, 
            [](auto commandBuffer)
            {
                commandBuffer->Clear(LLGL::ClearFlags::ColorDepth);
            },
            nullptr,
            renderer->GetPrimaryRenderTarget()
        );
}

void Scene::RenderSky(LLGL::RenderTarget* renderTarget)
{
    auto hdriSkyView = registry.view<MeshComponent, MeshRendererComponent, PipelineComponent, HDRISkyComponent>();

    if(hdriSkyView.begin() != hdriSkyView.end())
    {
        auto [mesh, meshRenderer, pipeline] = 
            hdriSkyView.get<MeshComponent, MeshRendererComponent, PipelineComponent>(*hdriSkyView.begin());

        MeshRenderPass(mesh, meshRenderer, pipeline, renderTarget);

        return;
    }

    auto proceduralSkyView = registry.view<MeshComponent, ProceduralSkyComponent>();

    if(proceduralSkyView.begin() != proceduralSkyView.end())
    {
        auto [mesh, sky] = proceduralSkyView.get<MeshComponent, ProceduralSkyComponent>(*proceduralSkyView.begin());

        ProceduralSkyRenderPass(mesh, sky, renderTarget);
    }
}

void Scene::MeshRenderPass(MeshComponent mesh, MeshRendererComponent meshRenderer, PipelineComponent pipeline, LLGL::RenderTarget* renderTarget)
{
    for(size_t i = 0; i < mesh.meshes.size(); i++)
    {
        //auto material = TextureManager::Get().GetDefaultTexture();
        auto material = AssetManager::Get().Load<TextureAsset>("");

        if(meshRenderer.materials.size() > i)
            material = meshRenderer.materials[i];

        Renderer::Get().RenderPass(
            [&](auto commandBuffer)
            {
                mesh.meshes[i]->BindBuffers(commandBuffer);
            },
            {
                { 0, Renderer::Get().GetMatricesBuffer() },
                { 1, material->texture },
                { 2, material->sampler }
            },
            [&](auto commandBuffer)
            {
                mesh.meshes[i]->Draw(commandBuffer);
            },
            pipeline.pipeline,
            renderer->GetPrimaryRenderTarget()
        );
    }
}

void Scene::ProceduralSkyRenderPass(MeshComponent mesh, ProceduralSkyComponent sky, LLGL::RenderTarget* renderTarget)
{
    Renderer::Get().RenderPass(
        [&](auto commandBuffer)
        {
            mesh.meshes[0]->BindBuffers(commandBuffer);
        },
        {
            { 0, Renderer::Get().GetMatricesBuffer() }
        },
        [&](auto commandBuffer)
        {
            sky.setUniforms(commandBuffer);
            
            mesh.meshes[0]->Draw(commandBuffer);
        },
        sky.pipeline,
        renderTarget
    );
}

void Scene::RenderResult(LLGL::RenderTarget* renderTarget)
{
    static auto uniforms = [&](auto commandBuffer)
    {
        auto numLights = lights.size();
        commandBuffer->SetUniforms(0, &numLights, sizeof(numLights));
        commandBuffer->SetUniforms(1, &cameraPosition, sizeof(cameraPosition));
    };

    Renderer::Get().Begin();

    RenderSky(renderTarget);

    renderer->Draw(
        { { 3, lightsBuffer } },
        uniforms,
        renderTarget
    );

    Renderer::Get().End();

    Renderer::Get().Submit();
}

void Scene::ApplyPostProcessing()
{
    auto acesView = registry.view<ACESTonemappingComponent>();

    if(acesView->begin() == acesView->end())
    {
        RenderResult();
        return;
    }

    auto postProcessing = *acesView->begin();

    RenderResult(postProcessing.postProcessing->GetRenderTarget());

    postProcessing.postProcessing->Apply(
        {
            { 0, postProcessing.postProcessing->GetFrame() },
            { 1, AssetManager::Get().Load<TextureAsset>("")->sampler }
        },
        postProcessing.setUniforms,
        Renderer::Get().GetSwapChain()
    );
}
    
}
