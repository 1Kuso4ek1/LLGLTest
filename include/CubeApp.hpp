#pragma once
#include <Application.hpp>

class CubeApp : Application
{
public:
    CubeApp();

    void Run() override;

private:
    void LoadShaders();
    void LoadTextures();

    float degrees = 0.f;

    std::unique_ptr<Mesh> mesh;

    LLGL::PipelineState* pipeline{};

    LLGL::Shader* vertexShader{};
    LLGL::Shader* fragmentShader{};

    LLGL::Texture* texture{};
    LLGL::Sampler* sampler{};

    std::shared_ptr<Matrices> matrices;
};
