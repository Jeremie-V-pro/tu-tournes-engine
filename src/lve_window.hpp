#pragma once
#include <cstdint>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
namespace lve{
class LveWindow{

  public:
    LveWindow(int w, int h, std::string name);
    ~LveWindow();


    bool shouldClose() { return glfwWindowShouldClose(window);}
    VkExtent2D getExtend(){ return{ static_cast<uint32_t>(width), static_cast<uint32_t>(height)}; }
    bool wasWindowResized(){return framebufferResized;}
    void resetWindowResizedFlag(){framebufferResized = false;}
    GLFWwindow *getGLFWwindow() const{return window;}

    void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);

  private:
    static void framebufferResizeCallback(GLFWwindow *window, int width, int height);
    void initWindow();

    int width;
    int height;
    bool framebufferResized = false;

    std::string windowName;
    GLFWwindow *window;

};
}