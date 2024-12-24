#include <SceneTestApp.hpp>

SceneTestApp::SceneTestApp()
{
    LLGL::Log::RegisterCallbackStd(LLGL::Log::StdOutFlags::Colored);

    dev::ScopedTimer timer("Engine initialization");

    window = std::make_shared<dev::Window>(LLGL::Extent2D{ 1280, 720 }, "LLGLTest");

    if(!dev::Renderer::Get().IsInit())
        return;

    dev::Renderer::Get().InitSwapChain(window);

    dev::ImGuiManager::Get().Init(window->GetGLFWWindow(), "../resources/fonts/OpenSans-Regular.ttf");

    LoadShaders();
    LoadTextures();

    renderTarget = dev::Renderer::Get().CreateRenderTarget(window->GetContentSize(), frame->texture);

    (mesh = std::make_shared<dev::Mesh>())->CreateCube();

    pipeline = dev::Renderer::Get().CreatePipelineState(vertexShader, fragmentShader);
    matrices = dev::Renderer::Get().GetMatrices();

    /* matrices->GetProjection() = glm::perspective(glm::radians(90.0f), 1280.0f / 720.0f, 0.1f, 100.0f);
    matrices->GetView() = glm::lookAt(glm::vec3(0.f, 0.f, 5.f), { 0, 0, 0.f }, glm::vec3(0.f, 1.f, 0.f)); */

    CreateEntities();
}

SceneTestApp::~SceneTestApp()
{
    if(dev::Renderer::Get().IsInit())
    {
        dev::ImGuiManager::Get().Destroy();

        dev::Renderer::Get().Unload();
    }
}

void SceneTestApp::Run()
{
    if(!dev::Renderer::Get().IsInit())
    {
        LLGL::Log::Errorf("Error: Renderer is not initialized\n");
        return;
    }

    while(window->PollEvents())
    {
        LLGL::Surface::ProcessEvents();
        
        dev::Multithreading::Get().Update();

        Draw();

        if(dev::Keyboard::IsKeyPressed(dev::Keyboard::Key::Escape))
            break;

        if(dev::Keyboard::IsKeyPressed(dev::Keyboard::Key::F11) && keyboardTimer.GetElapsedSeconds() > 0.2f)
        {
            window->SetFullscreen(!window->IsFullscreen());
            keyboardTimer.Reset();
        }

        if(dev::Mouse::IsButtonPressed(dev::Mouse::Button::Right))
        {
            degrees = (dev::Mouse::GetPosition().x - 640.0) / 10.0;
            entity.GetComponent<dev::TransformComponent>().rotation.y = degrees;
        }
    }
}

void SceneTestApp::LoadShaders()
{
    vertexShader = dev::Renderer::Get().CreateShader(LLGL::ShaderType::Vertex, "../shaders/vertex.vert");
    fragmentShader = dev::Renderer::Get().CreateShader(LLGL::ShaderType::Fragment, "../shaders/fragment.frag");
}

void SceneTestApp::LoadTextures()
{
    texture = dev::TextureManager::Get().LoadTexture("../resources/textures/tex.jpg");
    sampler = dev::TextureManager::Get().GetAnisotropySampler();

    frame = dev::TextureManager::Get().CreateTexture(
        {
            .type = LLGL::TextureType::Texture2D,
            .format = LLGL::Format::RGBA8UNorm,
            .extent = LLGL::Extent3D{ window->GetContentSize().width, window->GetContentSize().height, 1 },
            .mipLevels = 1,
            .samples = 1
        }
    );

}

void SceneTestApp::CreateEntities()
{
    entity = scene.CreateEntity();

    entity.AddComponent<dev::NameComponent>().name = "Cube";
    entity.AddComponent<dev::TransformComponent>();
    entity.AddComponent<dev::MeshComponent>().meshes.push_back(mesh);
    entity.AddComponent<dev::MeshRendererComponent>().materials.push_back(texture);
    entity.AddComponent<dev::PipelineComponent>().pipeline = pipeline;

    camera = scene.CreateEntity();

    camera.AddComponent<dev::NameComponent>().name = "Camera";
    camera.AddComponent<dev::TransformComponent>().position = { 0.0f, 0.0f, 5.0f };
    camera.AddComponent<dev::CameraComponent>().camera.SetViewport(window->GetContentSize());
    
    camera.GetComponent<dev::CameraComponent>().camera.SetPerspective();
}

void SceneTestApp::DrawImGui()
{
    dev::ImGuiManager::Get().NewFrame();

    ImGui::ShowDemoWindow();

    ImGui::ShowMetricsWindow();

    ImGui::Begin("Scene");

    if(ImGui::CollapsingHeader("Cube"))
    {
        ImGui::Indent();
        dev::DrawEntityUI<dev::NameComponent, dev::TransformComponent>(scene.GetRegistry(), entity);
        ImGui::Unindent();
    }

    if(ImGui::CollapsingHeader("Camera"))
    {
        ImGui::Indent();
        dev::DrawEntityUI<dev::NameComponent, dev::TransformComponent, dev::CameraComponent>(scene.GetRegistry(), camera);
        ImGui::Unindent();
    }
    
    ImGui::End();

    static uint32_t selectedImage = 0;

    if(dev::Keyboard::IsKeyPressed(dev::Keyboard::Key::Tab) && keyboardTimer.GetElapsedSeconds() > 0.2f)
    {
        selectedImage++;
        keyboardTimer.Reset();
    }

    ImGui::Begin("Image");

    ImGui::Image(selectedImage, ImVec2(256, 256));
    
    ImGui::End();

    dev::ImGuiManager::Get().Render();
}

void SceneTestApp::Draw()
{
    scene.Draw(renderTarget);
    scene.Draw();

    DrawImGui();

    dev::Renderer::Get().Present();
}
