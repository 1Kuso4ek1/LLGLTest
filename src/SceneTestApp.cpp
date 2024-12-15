#include <SceneTestApp.hpp>

SceneTestApp::SceneTestApp()
{
    LLGL::Log::RegisterCallbackStd(LLGL::Log::StdOutFlags::Colored);

    dev::ScopedTimer timer("Engine initialization");

    window = std::make_shared<dev::Window>(LLGL::Extent2D{ 1280, 720 }, "LLGLTest");

    if(!dev::Renderer::Get().IsInit())
        return;

    dev::Renderer::Get().InitSwapChain(window);

    LoadShaders();
    LoadTextures();

    dev::ImGuiManager::Get().Init(window->GetGLFWWindow(), "../resources/fonts/OpenSans-Regular.ttf");

    (mesh = std::make_shared<dev::Mesh>())->CreateCube();

    pipeline = dev::Renderer::Get().CreatePipelineState(vertexShader, fragmentShader);
    matrices = dev::Renderer::Get().GetMatrices();

    matrices->GetProjection() = glm::perspective(glm::radians(90.0f), 1280.0f / 720.0f, 0.1f, 100.0f);
    matrices->GetView() = glm::lookAt(glm::vec3(0.f, 0.f, 5.f), { 0, 0, 0.f }, glm::vec3(0.f, 1.f, 0.f));

    entity = scene.CreateEntity();

    entity.AddComponent<dev::NameComponent>().name = "Cube";
    entity.AddComponent<dev::TransformComponent>();
    entity.AddComponent<dev::MeshComponent>().meshes.push_back(mesh);
    entity.AddComponent<dev::MaterialComponent>().albedo.push_back(texture);
    entity.AddComponent<dev::PipelineComponent>().pipeline = pipeline;
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
            degrees = (dev::Mouse::GetPosition().x - 640.0) / 100.0;

        entity.GetComponent<dev::TransformComponent>().rotation = glm::rotate(glm::mat4(1.0f), glm::radians(angle), axis);
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
}

void SceneTestApp::DrawImGui()
{
    dev::ImGuiManager::Get().NewFrame();

    ImGui::ShowDemoWindow();

    ImGui::Begin("Cube Rotation");

    ImGui::Text("Choose Axis for Rotation:");
    ImGui::SliderFloat3("Axis", &axis[0], -1.0f, 1.0f);
    
    ImGui::Text("Choose Angle for Rotation:");
    ImGui::SliderFloat("Angle", &angle, 0.0f, 360.0f);

    ImGui::End();

    ImGui::Begin("Scene");

    dev::DrawEntityUI<dev::NameComponent, dev::TransformComponent>(scene.GetRegistry(), entity);
    
    ImGui::End();

    dev::ImGuiManager::Get().Render();
}

void SceneTestApp::Draw()
{
    scene.Draw();

    DrawImGui();

    dev::Renderer::Get().Present();
}
