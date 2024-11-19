#include <GLFW/glfw3.h>
#include <Window.hpp>

bool glfw::Window::glfwInitialized = false;

glfw::Window::Window(const LLGL::Extent2D& size, const std::string_view& title)
    : size(size), title(title)
{
    if(!glfwInitialized)
    {
        #ifdef __linux__
            glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11); // So we don't use wayland
        #endif

        glfwInit();

        glfwInitialized = true;

        window = CreateWindow();
    }
}

glfw::Window::~Window()
{
    glfwDestroyWindow(window);
}

void glfw::Window::SwapBuffers()
{
    glfwSwapBuffers(window);
}

bool glfw::Window::PollEvents()
{
    glfwPollEvents();

    return !glfwWindowShouldClose(window);
}

void glfw::Window::ResetPixelFormat()
{
    glfwDestroyWindow(window);
    window = CreateWindow();
}

bool glfw::Window::GetNativeHandle(void* nativeHandle, size_t size)
{
    if(nativeHandle && size == sizeof(LLGL::NativeHandle) && window)
    {
        auto nativeHandlePtr = reinterpret_cast<LLGL::NativeHandle*>(nativeHandle);

        #ifdef _WIN32
            nativeHandlePtr->window = glfwGetWin32Window(window);
        #elif __APPLE__
            nativeHandlePtr->window = glfwGetCocoaWindow(window);
        #elif __linux__
            nativeHandlePtr->window = glfwGetX11Window(window);
        #endif

        nativeHandlePtr->display = glfwGetX11Display();

        return true;
    }

    return false;
}

bool glfw::Window::AdaptForVideoMode(LLGL::Extent2D* resolution, bool* fullscreen)
{
    size = *resolution;
    
    glfwSetWindowSize(window, size.width, size.height);

    return true;
}

LLGL::Extent2D glfw::Window::GetContentSize() const
{
    return size;
}

LLGL::Display* glfw::Window::FindResidentDisplay() const
{
    return LLGL::Display::GetPrimary();
}

GLFWwindow* glfw::Window::CreateWindow()
{
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    auto window = glfwCreateWindow(size.width, size.height, title.data(), nullptr, nullptr);

    if(!window)
        throw std::runtime_error("Failed to create GLFW window");

    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    return window;
}
