#include <PBRManager.hpp>

namespace dev
{

PBRManager::PBRManager()
{
    SetupConvertPipeline();
    SetupIrradiancePipeline();
    SetupPrefilteredPipeline();
    SetupBRDFPipeline();

    CreateBRDFTexture({ 256, 256 });
    CreateBRDFRenderTarget({ 256, 256 });

    RenderBRDF();
}

EnvironmentAssetPtr PBRManager::Build(
    const LLGL::Extent2D& resolution,
    TextureAssetPtr environmentMap,
    EnvironmentAssetPtr environmentAsset
)
{
    if(!cubeMap || !environmentAsset || resolution != renderTargets[0]->GetResolution())
        CreateCubemaps(resolution);

    ReleaseRenderTargets();
    CreateRenderTargets(resolution, cubeMap);

    RenderCubeMap(
        {
            { 1, environmentMap->texture },
            { 2, environmentMap->sampler }
        },
        cubeMap,
        pipelineConvert
    );

    ReleaseRenderTargets();
    CreateRenderTargets({ resolution.width / 32, resolution.height / 32 }, irradiance);

    RenderCubeMap(
        {
            { 1, cubeMap },
            { 2, environmentMap->sampler }
        },
        irradiance,
        pipelineIrradiance
    );

    ReleaseRenderTargets();
    CreateRenderTargets(resolution, prefiltered);

    RenderCubeMapMips(
        {
            { 1, cubeMap },
            { 2, environmentMap->sampler }
        },
        prefiltered,
        pipelinePrefiltered
    );

    if(environmentAsset)
    {
        environmentAsset->cubeMap = cubeMap;
        environmentAsset->irradiance = irradiance;
        environmentAsset->prefiltered = prefiltered;
        environmentAsset->brdf = brdf;
    }
    else
        environmentAsset = std::make_shared<EnvironmentAsset>(cubeMap, irradiance, prefiltered, brdf);

    environmentAsset->loaded = true;

    return environmentAsset;
}

void PBRManager::RenderCubeMap(
    const std::unordered_map<uint32_t, LLGL::Resource*>& resources,
    LLGL::Texture* cubeMap,
    LLGL::PipelineState* pipeline
)
{    
    auto matrices = Renderer::Get().GetMatrices();

    matrices->PushMatrix();

    matrices->GetModel() = glm::mat4(1.0f);
    matrices->GetProjection() = projection;

    auto cube = AssetManager::Get().Load<ModelAsset>("cube", true)->meshes[0];
    
    for(int i = 0; i < 6; i++)
    {
        matrices->GetView() = views[i];

        Renderer::Get().Begin();

        Renderer::Get().RenderPass(
            [&](auto commandBuffer)
            {
                cube->BindBuffers(commandBuffer);
            },
            {
                { 0, Renderer::Get().GetMatricesBuffer() },
                { 1, resources.at(1) },
                { 2, resources.at(2) }
            },
            [&](auto commandBuffer)
            {
                cube->Draw(commandBuffer);
            },
            pipeline,
            renderTargets[i]
        );

        Renderer::Get().End();
        
        Renderer::Get().Submit();
    }

    Renderer::Get().GenerateMips(cubeMap);

    matrices->PopMatrix();
}

void PBRManager::RenderCubeMapMips(
    const std::unordered_map<uint32_t, LLGL::Resource*>& resources,
    LLGL::Texture* cubeMap,
    LLGL::PipelineState* pipeline
)
{
    auto matrices = Renderer::Get().GetMatrices();

    matrices->PushMatrix();

    matrices->GetModel() = glm::mat4(1.0f);
    matrices->GetProjection() = projection;

    auto cube = AssetManager::Get().Load<ModelAsset>("cube", true)->meshes[0];

    auto initialResolution = renderTargets[0]->GetResolution();

    auto mipsNum = 
        LLGL::NumMipLevels(
            initialResolution.width,
            initialResolution.height
        );

    for(int i = 0; i < mipsNum; i++)
    {
        for(int j = 0; j < 6; j++)
        {
            matrices->GetView() = views[j];

            Renderer::Get().Begin();

            Renderer::Get().RenderPass(
                [&](auto commandBuffer)
                {
                    cube->BindBuffers(commandBuffer);
                },
                {
                    { 0, Renderer::Get().GetMatricesBuffer() },
                    { 1, resources.at(1) },
                    { 2, resources.at(2) }
                },
                [&](auto commandBuffer)
                {
                    float roughness = (float)i / (float)(mipsNum - 1);

                    commandBuffer->SetUniforms(0, &roughness, sizeof(roughness));

                    cube->Draw(commandBuffer);
                },
                pipeline,
                renderTargets[j]
            );

            Renderer::Get().End();

            Renderer::Get().Submit();
        }

        ReleaseRenderTargets();
        CreateRenderTargets(
            {
                initialResolution.width / (int)std::pow(2, i),
                initialResolution.height / (int)std::pow(2, i)
            },
            prefiltered,
            i
        );
    }

    matrices->PopMatrix();
}

void PBRManager::RenderBRDF()
{
    auto plane = AssetManager::Get().Load<ModelAsset>("plane", true)->meshes[0];

    Renderer::Get().Begin();

    Renderer::Get().RenderPass(
        [&](auto commandBuffer)
        {
            plane->BindBuffers(commandBuffer, false);
        },
        {},
        [&](auto commandBuffer)
        {
            plane->Draw(commandBuffer);
        },
        pipelineBRDF,
        brdfRenderTarget
    );

    Renderer::Get().End();

    Renderer::Get().Submit();
}

void PBRManager::SetupConvertPipeline()
{
    pipelineConvert = Renderer::Get().CreatePipelineState(
        LLGL::PipelineLayoutDescriptor
        {
            .bindings =
            {
                { "matrices", LLGL::ResourceType::Buffer, LLGL::BindFlags::ConstantBuffer, LLGL::StageFlags::VertexStage, 1 },
                { "hdri", LLGL::ResourceType::Texture, LLGL::BindFlags::Sampled, LLGL::StageFlags::FragmentStage, 2 },
                { "samplerState", LLGL::ResourceType::Sampler, 0, LLGL::StageFlags::FragmentStage, 2 }
            }
        },
        LLGL::GraphicsPipelineDescriptor
        {
            .vertexShader = Renderer::Get().CreateShader(LLGL::ShaderType::Vertex, "../shaders/skybox.vert"),
            .fragmentShader = Renderer::Get().CreateShader(LLGL::ShaderType::Fragment, "../shaders/HDRIConvert.frag"),
            .depth = LLGL::DepthDescriptor
            {
                .testEnabled = true,
                .writeEnabled = false,
                .compareOp = LLGL::CompareOp::LessEqual
            },
            .rasterizer = LLGL::RasterizerDescriptor
            {
                .cullMode = LLGL::CullMode::Front,
                .frontCCW = true
            }
        }
    );
}

void PBRManager::SetupIrradiancePipeline()
{
    pipelineIrradiance = Renderer::Get().CreatePipelineState(
        LLGL::PipelineLayoutDescriptor
        {
            .bindings =
            {
                { "matrices", LLGL::ResourceType::Buffer, LLGL::BindFlags::ConstantBuffer, LLGL::StageFlags::VertexStage, 1 },
                { "skybox", LLGL::ResourceType::Texture, LLGL::BindFlags::Sampled, LLGL::StageFlags::FragmentStage, 2 },
                { "samplerState", LLGL::ResourceType::Sampler, 0, LLGL::StageFlags::FragmentStage, 2 }
            }
        },
        LLGL::GraphicsPipelineDescriptor
        {
            .vertexShader = Renderer::Get().CreateShader(LLGL::ShaderType::Vertex, "../shaders/skybox.vert"),
            .fragmentShader = Renderer::Get().CreateShader(LLGL::ShaderType::Fragment, "../shaders/irradiance.frag"),
            .depth = LLGL::DepthDescriptor
            {
                .testEnabled = true,
                .writeEnabled = false,
                .compareOp = LLGL::CompareOp::LessEqual
            },
            .rasterizer = LLGL::RasterizerDescriptor
            {
                .cullMode = LLGL::CullMode::Front,
                .frontCCW = true
            }
        }
    );
}

void PBRManager::SetupPrefilteredPipeline()
{
    pipelinePrefiltered = Renderer::Get().CreatePipelineState(
        LLGL::PipelineLayoutDescriptor
        {
            .bindings =
            {
                { "matrices", LLGL::ResourceType::Buffer, LLGL::BindFlags::ConstantBuffer, LLGL::StageFlags::VertexStage, 1 },
                { "skybox", LLGL::ResourceType::Texture, LLGL::BindFlags::Sampled, LLGL::StageFlags::FragmentStage, 2 },
                { "samplerState", LLGL::ResourceType::Sampler, 0, LLGL::StageFlags::FragmentStage, 2 }
            },
            .uniforms = 
            {
                { "roughness", LLGL::UniformType::Float1 }
            }
        },
        LLGL::GraphicsPipelineDescriptor
        {
            .vertexShader = Renderer::Get().CreateShader(LLGL::ShaderType::Vertex, "../shaders/skybox.vert"),
            .fragmentShader = Renderer::Get().CreateShader(LLGL::ShaderType::Fragment, "../shaders/prefilter.frag"),
            .depth = LLGL::DepthDescriptor
            {
                .testEnabled = true,
                .writeEnabled = false,
                .compareOp = LLGL::CompareOp::LessEqual
            },
            .rasterizer = LLGL::RasterizerDescriptor
            {
                .cullMode = LLGL::CullMode::Front,
                .frontCCW = true
            }
        }
    );
}

void PBRManager::SetupBRDFPipeline()
{
    pipelineBRDF = Renderer::Get().CreatePipelineState({},
        {
            .vertexShader = Renderer::Get().CreateShader(LLGL::ShaderType::Vertex, "../shaders/screenRect.vert"),
            .fragmentShader = Renderer::Get().CreateShader(LLGL::ShaderType::Fragment, "../shaders/BRDF.frag"),
            .primitiveTopology = LLGL::PrimitiveTopology::TriangleList
        }
    );
}

void PBRManager::CreateCubemaps(const LLGL::Extent2D& resolution)
{
    LLGL::TextureDescriptor textureDesc
    {
        .type = LLGL::TextureType::TextureCubeArray,
        .bindFlags = LLGL::BindFlags::ColorAttachment | LLGL::BindFlags::Sampled,
        .format = LLGL::Format::RGB16Float,
        .extent = { resolution.width, resolution.height, 1 },
        .arrayLayers = 6
    };

    cubeMap = Renderer::Get().CreateTexture(textureDesc);
    Renderer::Get().GenerateMips(cubeMap);

    textureDesc.extent = { resolution.width / 32, resolution.height / 32, 1 };
    irradiance = Renderer::Get().CreateTexture(textureDesc);

    textureDesc.extent = { resolution.width, resolution.height, 1 };
    prefiltered = Renderer::Get().CreateTexture(textureDesc);
    Renderer::Get().GenerateMips(prefiltered);
    

    // LLGL::NumMipLevels(resolution.width, resolution.height);
}

void PBRManager::CreateRenderTargets(const LLGL::Extent2D& resolution, LLGL::Texture* cubeMap, int mipLevel)
{
    for(int i = 0; i < 6; i++)
    {
        renderTargets[i] = Renderer::Get().CreateRenderTarget(
            resolution,
            { LLGL::AttachmentDescriptor(cubeMap, mipLevel, i) }
        );
    }
}

void PBRManager::CreateBRDFTexture(const LLGL::Extent2D& resolution)
{
    LLGL::TextureDescriptor textureDesc
    {
        .type = LLGL::TextureType::Texture2D,
        .bindFlags = LLGL::BindFlags::ColorAttachment | LLGL::BindFlags::Sampled,
        .format = LLGL::Format::RG16Float,
        .extent = { resolution.width, resolution.height, 1 },
        .mipLevels = 1
    };

    brdf = Renderer::Get().CreateTexture(textureDesc);
}

void PBRManager::CreateBRDFRenderTarget(const LLGL::Extent2D& resolution)
{
    brdfRenderTarget = Renderer::Get().CreateRenderTarget(resolution, { brdf });
}

void PBRManager::ReleaseCubeMaps()
{
    if(cubeMap)
    {
        Renderer::Get().Release(cubeMap);
        Renderer::Get().Release(irradiance);
        Renderer::Get().Release(prefiltered);
    }
}

void PBRManager::ReleaseRenderTargets()
{
    for(auto renderTarget : renderTargets)
        if(renderTarget)
            Renderer::Get().Release(renderTarget);
}

}
