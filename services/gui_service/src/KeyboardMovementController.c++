#include "KeyboardMovementController.hpp"
#include <glm/geometric.hpp>
#include <limits>
#include <iostream>
namespace et{
    void KeyboardMovementController::moveInPlaneXZ(GLFWwindow* window, float dt, EtGameObject &gameObject, glm::vec3& location){
        glm::vec3 rotate{0};
        if(gameObject.name == "et"){
            if(glfwGetKey(window, keys.lookRight) == GLFW_PRESS) rotate.y += 1.f;
            if(glfwGetKey(window, keys.lookLeft) == GLFW_PRESS) rotate.y -= 1.f;
            if(glfwGetKey(window, keys.lookUp) == GLFW_PRESS) rotate.y += 1.f;
            if(glfwGetKey(window, keys.lookDown) == GLFW_PRESS) rotate.y -= 1.f;
            if(glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon()) gameObject.transform.rotation += lookSpeed * dt * glm::normalize(rotate);
        }
        gameObject.transform.rotation.x = glm::clamp(gameObject.transform.rotation.x, -1.5f, 1.5f);
        gameObject.transform.rotation.y = glm::mod(gameObject.transform.rotation.y, glm::two_pi<float>());
        float yaw = gameObject.transform.rotation.y;
        const glm::vec3 forwardDir{sin(yaw), 0.f, cos(yaw)};
        const glm::vec3 rightDir{forwardDir.z, 0.f, -forwardDir.x};
        const glm::vec3 upDir{0.f, -1.f, 0.f};
        glm::vec3 moveDir{0.f};
        if(glfwGetKey(window, keys.moveForward) == GLFW_PRESS) moveDir += forwardDir;
        if(glfwGetKey(window, keys.moveBackward) == GLFW_PRESS) moveDir -= forwardDir;
        if(glfwGetKey(window, keys.moveRight) == GLFW_PRESS) moveDir += rightDir;
        if(glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) moveDir -= rightDir;
        if(glfwGetKey(window, keys.moveUp) == GLFW_PRESS) moveDir += upDir;
        if(glfwGetKey(window, keys.moveDown) == GLFW_PRESS) moveDir -= upDir;
        if(glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) gameObject.transform.translation += moveSpeed * dt * glm::normalize(moveDir);
        if(gameObject.name == "et"){
            glm::vec3 gridOrigin = {0.0f - 5, 7.0f, 13.0f - 5};
            float x = gameObject.transform.translation.x;
            float z = gameObject.transform.translation.z;
            float worldX = x;
            float worldZ = z;
            float localGridX = worldX - gridOrigin.x;
            float localGridZ = worldZ - gridOrigin.z;
            int tileX = static_cast<int>(floor(localGridX / 10.0f));
            int tileZ = static_cast<int>(floor(localGridZ / 10.0f));
            float tileLocalX = localGridX - tileX * 10.0f;
            float tileLocalZ = localGridZ - tileZ * 10.0f;
            std::cout<<"X: "<<std::to_string(tileLocalX)<<"\nZ: "<<std::to_string(tileLocalZ)<<std::endl;
            location = gameObject.transform.translation;
        }
    }
}