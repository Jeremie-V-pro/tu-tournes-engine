#pragma once

#include "lve_game_object.hpp"

namespace lve {
class KeyboardMouvementController {
   public:
    struct KeyMappings {
        int moveLeft = GLFW_KEY_A;
        int moveRight = GLFW_KEY_D;
        int moveForward = GLFW_KEY_W;
        int moveBackward = GLFW_KEY_S;
        int moveUp = GLFW_KEY_E;
        int moveDown = GLFW_KEY_Q;
        int lookLeft = GLFW_KEY_LEFT;
        int lookRight = GLFW_KEY_RIGHT;
        int lookUp = GLFW_KEY_UP;
        int lookDown = GLFW_KEY_DOWN;
        int escape = GLFW_KEY_ESCAPE;
    };

    void moveInPlaneXZ(GLFWwindow* window, float dt, LveGameObject& gameObject);
    static void mouse_callback(GLFWwindow* window, double xpos, double ypos);

    KeyMappings keys{};
    float moveSpeed{3.f};
    float lookSpeed{1.5f};

    static bool firstMouse;
    static double lastX;
    static double lastY;
    static float yaw;
    static float pitch;
    static glm::vec3 cameraFront;
};
}  // namespace lve