#include <DeferredRenderer.hpp>

namespace dev
{

DeferredRenderer::DeferredRenderer(const LLGL::Extent2D& resolution)
{
    LLGL::TextureDescriptor colorAttachmentDesc =
    {
        .type = LLGL::TextureType::Texture2D,
        .bindFlags = LLGL::BindFlags::ColorAttachment,
        .format = LLGL::Format::RGBA16Float,
        .extent = { resolution.width, resolution.height, 1 },
        .mipLevels = 1,
        .samples = 1
    };

    LLGL::TextureDescriptor depthAttachmentDesc =
    {
        .type = LLGL::TextureType::Texture2D,
        .bindFlags = LLGL::BindFlags::DepthStencilAttachment,
        .format = LLGL::Format::D32Float,
        .extent = { resolution.width, resolution.height, 1 },
        .mipLevels = 1,
        .samples = 1
    };

    gBufferPosition = Renderer::Get().CreateTexture(colorAttachmentDesc);
    gBufferAlbedo = Renderer::Get().CreateTexture(colorAttachmentDesc);
    gBufferNormal = Renderer::Get().CreateTexture(colorAttachmentDesc);

    gBufferDepth = Renderer::Get().CreateTexture(depthAttachmentDesc);

    gBuffer = Renderer::Get().CreateRenderTarget(resolution, { gBufferPosition, gBufferAlbedo, gBufferNormal }, gBufferDepth);
    gBufferPipeline = Renderer::Get().CreateRenderTargetPipeline(gBuffer);

    /* lightingPipeline = Renderer::Get().CreatePipelineState(
        LLGL::PipelineLayoutDescriptor{},
        LLGL::GraphicsPipelineDescriptor{}
    ); */

    rect = std::make_shared<Mesh>();
    rect->CreatePlane();

    rectPipeline = Renderer::Get().CreatePipelineState(
        LLGL::PipelineLayoutDescriptor
        {
            .bindings =
            {
                { "gPosition", LLGL::ResourceType::Texture, LLGL::BindFlags::Sampled, LLGL::StageFlags::FragmentStage, 1 },
                { "gAlbedo", LLGL::ResourceType::Texture, LLGL::BindFlags::Sampled, LLGL::StageFlags::FragmentStage, 2 },
                { "gNormal", LLGL::ResourceType::Texture, LLGL::BindFlags::Sampled, LLGL::StageFlags::FragmentStage, 3 },
                { "samplerState", LLGL::ResourceType::Sampler, 0, LLGL::StageFlags::FragmentStage, 1 }
            }
        },
        LLGL::GraphicsPipelineDescriptor
        {
            .renderPass = Renderer::Get().GetSwapChain()->GetRenderPass(),
            .vertexShader = Renderer::Get().CreateShader(LLGL::ShaderType::Vertex, "../shaders/post.vert"),
            .fragmentShader = Renderer::Get().CreateShader(LLGL::ShaderType::Fragment, "../shaders/lightingPass.frag"),
            .primitiveTopology = LLGL::PrimitiveTopology::TriangleList
        }
    );
}

void DeferredRenderer::Draw(LLGL::RenderTarget* renderTarget)
{
    Renderer::Get().Begin();

    Renderer::Get().RenderPass(
        [&](auto commandBuffer)
        {
            rect->BindBuffers(commandBuffer, false);
        },
        {
            { 0, gBufferPosition },
            { 1, gBufferAlbedo },
            { 2, gBufferNormal },
            { 3, TextureManager::Get().GetAnisotropySampler() }
        },
        [&](auto commandBuffer)
        {
            rect->Draw(commandBuffer);
        },
        rectPipeline,
        renderTarget
    );

    Renderer::Get().End();

    Renderer::Get().Submit();
}

LLGL::RenderTarget* DeferredRenderer::GetPrimaryRenderTarget()
{
    return gBuffer;
}

}