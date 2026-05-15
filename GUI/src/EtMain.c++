#include "EtGameObject.hpp"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include "EtMain.hpp"
#include "SimpleRenderSystem.hpp"
#include "KeyboardMovementController.hpp"
#include <GLFW/glfw3.h>
#include <memory>
#include <vulkan/vulkan_core.h>
#include <glm/gtc/constants.hpp>
#include "EtCamera.hpp"
#include <chrono>
namespace et{
    EtMain::EtMain(){
        loadGameObjects();
    }
    EtMain::~EtMain(){}
    void EtMain::run(){
        SimpleRenderSystem simpleRenderSystem{etDevice, etRenderer.getSwapChainRenderPass()};
        EtCamera camera{};
        camera.setViewTarget(glm::vec3(-1.f, -2.f, 2.f), glm::vec3(0.f, 0.f, 2.5f));
        auto viewerObject = EtGameObject::createGameObject();
        KeyboardMovmentController cameraController{};
        auto currentTime = std::chrono::high_resolution_clock::now();
        while(!etWindow.shouldClose()){
            glfwPollEvents();
            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            currentTime = newTime;
            cameraController.moveInPlaneXZ(etWindow.getGLFWwindow(), frameTime, viewerObject);
            camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);
            float aspect = etRenderer.getAspectRatio();
            camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1, 30);
            if(auto commandBuffer = etRenderer.beginFrame()){
                etRenderer.beginSwapChainRenderPass(commandBuffer);
                simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects, camera);
                etRenderer.endSwapChainRenderPass(commandBuffer);
                etRenderer.endFrame();
            }
        }
        vkDeviceWaitIdle(etDevice.device());
    }
    void EtMain::loadGameObjects(){
        std::shared_ptr<EtModel> etModel = EtModel::createModelFromFile(etDevice, "assets/scenes/Forest.obj");
        auto Tile = EtGameObject::createGameObject();
        Tile.model = etModel;
        Tile.transform.translation = {0.0f, 0.0f, 2.5f};
        Tile.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back((std::move(Tile)));
    }
}