#include "EtGameObject.hpp"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include "EtMain.hpp"
#include "SimpleRenderSystem.hpp"
#include <GLFW/glfw3.h>
#include <memory>
#include <vulkan/vulkan_core.h>
#include <glm/gtc/constants.hpp>
namespace et{
    EtMain::EtMain(){
        loadGameObjects();
    }
    EtMain::~EtMain(){}
    void EtMain::run(){
        SimpleRenderSystem simpleRenderSystem{etDevice, etRenderer.getSwapChainRenderPass()};
        while(!etWindow.shouldClose()){
            glfwPollEvents();
            if(auto commandBuffer = etRenderer.beginFrame()){
                etRenderer.beginSwapChainRenderPass(commandBuffer);
                simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects);
                etRenderer.endSwapChainRenderPass(commandBuffer);
                etRenderer.endFrame();
            }
        }
        vkDeviceWaitIdle(etDevice.device());
    }
    void EtMain::loadGameObjects(){
        std::vector<EtModel::Vertex> vertices{
            {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
            {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
        };
        auto etModel = std::make_shared<EtModel>(etDevice, vertices);
        auto triangle = EtGameObject::createGameObject();
        triangle.model = etModel;
        triangle.color = {0.1f, 0.8f, 0.1f};
        triangle.transform2d.translation.x = 0.2f;
        triangle.transform2d.scale = {2.f, 0.5};
        triangle.transform2d.rotation = 0.25f * glm::two_pi<float>();
        gameObjects.push_back(std::move(triangle));
    }
}