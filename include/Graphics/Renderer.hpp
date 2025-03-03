#pragma once
#include <Utils.hpp>
#include <Singleton.hpp>

#include <LLGL/Surface.h>

#include <filesystem>
#include <functional>
#include <memory>
#include <unordered_map>

namespace lustra
{

class Renderer : public Singleton<Renderer>
{
public: // Public methods
    void Init();

    void InitSwapChain(const LLGL::Extent2D& resolution, bool fullscreen = false, int samples = 1);
    void InitSwapChain(std::shared_ptr<LLGL::Surface> surface);

    void Begin(); // Start writing to the command buffer
    void End(); // End writing to the command buffer

    void RenderPass(
        std::function<void(LLGL::CommandBuffer*)> setupBuffers, // Set vert/ind/static buffers with CommandBuffer
        const std::unordered_map<uint32_t, LLGL::Resource*>& resources, // A map of resources { binding, Resource_ptr }
        std::function<void(LLGL::CommandBuffer*)> draw, // Call the draw function
        LLGL::PipelineState* pipeline,
        LLGL::RenderTarget* renderTarget = nullptr
    );

    void Submit();
    void Present();

    void ClearRenderTarget(LLGL::RenderTarget* renderTarget = nullptr, bool begin = true);

    void GenerateMips(LLGL::Texture* texture);

    // BRUH
    template<class T>
    void Release(T* resource)
    {
        renderSystem->Release(*resource);
    }

    void Unload();

    void WriteTexture(LLGL::Texture& texture, const LLGL::TextureRegion& textureRegion, const LLGL::ImageView& srcImageView);

    void SetViewportResolution(const LLGL::Extent2D& resolution);

    LLGL::Extent2D GetViewportResolution() const;

    LLGL::Buffer* CreateBuffer(const LLGL::BufferDescriptor& bufferDesc, const void* initialData = nullptr);
    LLGL::Buffer* CreateBuffer(const std::string& name, const LLGL::BufferDescriptor& bufferDesc, const void* initialData = nullptr);
    LLGL::Shader* CreateShader(const LLGL::ShaderType& type, const std::filesystem::path& path, const std::vector<LLGL::VertexAttribute>& attributes = {});
    LLGL::Texture* CreateTexture(const LLGL::TextureDescriptor& textureDesc, const LLGL::ImageView* initialImage = nullptr);
    LLGL::Sampler* CreateSampler(const LLGL::SamplerDescriptor& samplerDesc);
    LLGL::RenderTarget* CreateRenderTarget(const LLGL::Extent2D& resolution, const std::vector<LLGL::AttachmentDescriptor>& colorAttachments, LLGL::Texture* depthTexture = nullptr);

    LLGL::PipelineState* CreatePipelineState(LLGL::Shader* vertexShader, LLGL::Shader* fragmentShader);
    LLGL::PipelineState* CreatePipelineState(const LLGL::PipelineLayoutDescriptor& layoutDesc, LLGL::GraphicsPipelineDescriptor pipelineDesc);
    LLGL::PipelineState* CreateRenderTargetPipeline(LLGL::RenderTarget* renderTarget);

    LLGL::SwapChain* GetSwapChain() const;
    LLGL::Window* GetWindow() const;
    LLGL::VertexFormat GetDefaultVertexFormat() const;

    LLGL::Buffer* GetMatricesBuffer() const;
    std::shared_ptr<Matrices> GetMatrices() const;

    bool IsInit(); // Will return false if RenderSystem init failed

private: // Singleton-related
    Renderer();

    friend class Singleton<Renderer>;

private: // Private methods
    void LoadRenderSystem(const LLGL::RenderSystemDescriptor& desc);
    
    void SetupDefaultVertexFormat();
    void SetupCommandBuffer();
    void CreateMatricesBuffer();

    void SetupBuffers();

private: // Private members
    uint64_t renderPassCounter = 0;

    LLGL::Extent2D viewportResolution;

    LLGL::RenderSystemPtr renderSystem;

    LLGL::SwapChain* swapChain{};
    LLGL::CommandBuffer* commandBuffer{};
    LLGL::CommandQueue* commandQueue{};

    LLGL::VertexFormat defaultVertexFormat;

    LLGL::Buffer* matricesBuffer{};
    std::shared_ptr<Matrices> matrices;

    std::unordered_map<std::string, LLGL::Buffer*> globalBuffers;
    std::unordered_map<uint64_t, LLGL::PipelineState*> pipelineCache;
};

}
