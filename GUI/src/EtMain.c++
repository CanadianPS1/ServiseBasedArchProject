#include "EtFrameInfo.hpp"
#include "EtGameObject.hpp"
#include "EtSwapChain.hpp"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include "EtMain.hpp"
#include "EtBuffer.hpp"
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
         std::vector<std::unique_ptr<EtBuffer>> uboBuffers(EtSwapChain::MAX_FRAMES_IN_FLIGHT);
        for(int i = 0; i < uboBuffers.size(); i++){
          uboBuffers[i] = std::make_unique<EtBuffer>(etDevice, sizeof(GlobalUbo), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);  
            uboBuffers[i]->map();
        } 
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
                int frameIndex = etRenderer.getFrameIndex();
                FrameInfo frameInfo{frameIndex, frameTime, commandBuffer, camera};
                GlobalUbo ubo{};
                ubo.projectionView = camera.getProjection() * camera.getView();
                uboBuffers[frameIndex]->writeToBuffer(&ubo);
                uboBuffers[frameIndex]->flush();
                etRenderer.beginSwapChainRenderPass(commandBuffer);
                simpleRenderSystem.renderGameObjects(gameObjects, frameInfo);
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