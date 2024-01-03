#include "lve_window.hpp"

#include <GLFW/glfw3.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <wayland-cursor.h>
#include <wayland-egl.h>

#include <stdexcept>

namespace lve {
LveWindow::LveWindow(int w, int h, std::string name) : width{w}, height{h}, windowName{name} { initWindow(); }

LveWindow::~LveWindow() {
    glfwDestroyWindow(window);
    glfwTerminate();
}
void LveWindow::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
}

void LveWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR *surface) {
    if (glfwCreateWindowSurface(instance, window, nullptr, surface)) {
        throw std::runtime_error("failed to create window surface");
    }
}

void LveWindow::framebufferResizeCallback(GLFWwindow *window, int width, int height) {
    auto lveWindow = reinterpret_cast<LveWindow *>(glfwGetWindowUserPointer(window));
    lveWindow->framebufferResized = true;
    lveWindow->width = width;
    lveWindow->height = height;
}
}  // namespace lve