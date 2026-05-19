#include "EtDescriptors.hpp"
#include "EtFrameInfo.hpp"
#include "EtGameObject.hpp"
#include "EtSwapChain.hpp"
#include <glm/detail/qualifier.hpp>
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
    void setEtLocation(GlobalUbo& ubo, float x, float y, float z){ubo.EtLocation = {x,y,z};}
    glm::vec3 getEtLocation(GlobalUbo& ubo){return ubo.EtLocation;}
    EtMain::EtMain(){
        globalPool = EtDescriptorPool::Builder(etDevice)
            .setMaxSets(EtSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, EtSwapChain::MAX_FRAMES_IN_FLIGHT)
            .build();
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
        auto globalSetLayout = EtDescriptorSetLayout::Builder(etDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
            .build();
        std::vector<VkDescriptorSet> globalDescriptorSets(EtSwapChain::MAX_FRAMES_IN_FLIGHT);
        for(int i = 0; i < globalDescriptorSets.size(); i++){
            auto bufferInfo = uboBuffers[i]->descriptorInfo();
            EtDescriptorWriter(*globalSetLayout, *globalPool)
                .writeBuffer(0, &bufferInfo)
                .build(globalDescriptorSets[i]);
        }
        SimpleRenderSystem simpleRenderSystem{etDevice, etRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout()};
        EtCamera camera{};
        camera.setViewTarget(glm::vec3(-1.f, -2.f, 2.f), glm::vec3(0.f, 0.f, 2.5f));
        auto viewerObject = EtGameObject::createGameObject();
        viewerObject.transform.rotation = {-0.5f, 0.f, 0.f};
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
            camera.setPerspectiveProjection(glm::radians(20.f), aspect, 1, 30);
            if(auto commandBuffer = etRenderer.beginFrame()){
                int frameIndex = etRenderer.getFrameIndex();
                FrameInfo frameInfo{frameIndex, frameTime, commandBuffer, camera, globalDescriptorSets[frameIndex]};
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
        auto Forest = EtGameObject::createGameObject();
        Forest.model = etModel;
        Forest.transform.translation = {0.0f, 7.0f, 13.f};
        Forest.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back((std::move(Forest)));

        etModel = EtModel::createModelFromFile(etDevice, "assets/scenes/UltraHole.obj");
        auto UltraHole = EtGameObject::createGameObject();
        UltraHole.model = etModel;
        UltraHole.transform.translation = {10.f, 7.0f, 13.f};
        UltraHole.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back((std::move(UltraHole)));

        etModel = EtModel::createModelFromFile(etDevice, "assets/scenes/FourHoles.obj");
        auto FourCorners = EtGameObject::createGameObject();
        FourCorners.model = etModel;
        FourCorners.transform.translation = {-10.f, 7.0f, 13.f};
        FourCorners.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back((std::move(FourCorners)));

        etModel = EtModel::createModelFromFile(etDevice, "assets/scenes/DC.obj");
        auto DC = EtGameObject::createGameObject();
        DC.model = etModel;
        DC.transform.rotation = {0.f, 3.14159265f, 0.f};
        DC.transform.translation = {20.f, 7.0f, 13.f};
        DC.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back((std::move(DC)));

        etModel = EtModel::createModelFromFile(etDevice, "assets/scenes/DC.obj");
        auto DCDupe = EtGameObject::createGameObject();
        DCDupe.model = etModel;
        DCDupe.transform.rotation = {0.f, 3.14159265f, 0.f};
        DCDupe.transform.translation = {-20.f, 7.0f, 13.f};
        DCDupe.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back((std::move(DCDupe)));

        etModel = EtModel::createModelFromFile(etDevice, "assets/scenes/FourHoles.obj");
        auto FourCornersDupe = EtGameObject::createGameObject();
        FourCornersDupe.model = etModel;
        FourCornersDupe.transform.translation = {30.f, 7.0f, 13.f};
        FourCornersDupe.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back((std::move(FourCornersDupe)));

        etModel = EtModel::createModelFromFile(etDevice, "assets/scenes/TopHole.obj");
        auto OnTop = EtGameObject::createGameObject();
        OnTop.model = etModel;
        OnTop.transform.rotation = {0.f, 1.5708, 0.f};
        OnTop.transform.translation = {0.f, 7.0f, 23.f};
        OnTop.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back((std::move(OnTop)));

        etModel = EtModel::createModelFromFile(etDevice, "assets/scenes/TopHole.obj");
        auto OnTopDupe1 = EtGameObject::createGameObject();
        OnTopDupe1.model = etModel;
        OnTopDupe1.transform.rotation = {0.f, 1.5708f, 0.f};
        OnTopDupe1.transform.translation = {10.f, 7.0f, 23.f};
        OnTopDupe1.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back((std::move(OnTopDupe1)));
        
        etModel = EtModel::createModelFromFile(etDevice, "assets/scenes/TopHole.obj");
        auto OnTopDupe2 = EtGameObject::createGameObject();
        OnTopDupe2.model = etModel;
        OnTopDupe2.transform.rotation = {0.f, 1.5708f, 0.f};
        OnTopDupe2.transform.translation = {20.f, 7.0f, 23.f};
        OnTopDupe2.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back((std::move(OnTopDupe2)));

        etModel = EtModel::createModelFromFile(etDevice, "assets/scenes/TopHole.obj");
        auto OnTopDupe3 = EtGameObject::createGameObject();
        OnTopDupe3.model = etModel;
        OnTopDupe3.transform.rotation = {0.f, 1.5708f, 0.f};
        OnTopDupe3.transform.translation = {30.f, 7.0f, 23.f};
        OnTopDupe3.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back((std::move(OnTopDupe3)));

        etModel = EtModel::createModelFromFile(etDevice, "assets/scenes/TopHole.obj");
        auto OnTopDupe4 = EtGameObject::createGameObject();
        OnTopDupe4.model = etModel;
        OnTopDupe4.transform.rotation = {0.f, 1.5708f, 0.f};
        OnTopDupe4.transform.translation = {-10.f, 7.0f, 23.f};
        OnTopDupe4.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back((std::move(OnTopDupe4)));

        etModel = EtModel::createModelFromFile(etDevice, "assets/scenes/TopHole.obj");
        auto OnTopDupe5 = EtGameObject::createGameObject();
        OnTopDupe5.model = etModel;
        OnTopDupe5.transform.rotation = {0.f, 1.5708f, 0.f};
        OnTopDupe5.transform.translation = {-20.f, 7.0f, 23.f};
        OnTopDupe5.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back((std::move(OnTopDupe5)));

        etModel = EtModel::createModelFromFile(etDevice, "assets/scenes/TopHole.obj");
        auto OnTopB = EtGameObject::createGameObject();
        OnTopB.model = etModel;
        OnTopB.transform.rotation = {0.f, 1.5708f, 0.f};
        OnTopB.transform.translation = {0.f, 7.0f, -7.f};
        OnTopB.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back((std::move(OnTopB)));

        etModel = EtModel::createModelFromFile(etDevice, "assets/scenes/TopHole.obj");
        auto OnTopDupe1B = EtGameObject::createGameObject();
        OnTopDupe1B.model = etModel;
        OnTopDupe1B.transform.rotation = {0.f, 1.5708f, 0.f};
        OnTopDupe1B.transform.translation = {10.f, 7.0f, -7.f};
        OnTopDupe1B.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back((std::move(OnTopDupe1B)));
        
        etModel = EtModel::createModelFromFile(etDevice, "assets/scenes/TopHole.obj");
        auto OnTopDupe2B = EtGameObject::createGameObject();
        OnTopDupe2B.model = etModel;
        OnTopDupe2B.transform.rotation = {0.f, 1.5708f, 0.f};
        OnTopDupe2B.transform.translation = {20.f, 7.0f, -7.f};
        OnTopDupe2B.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back((std::move(OnTopDupe2B)));

        etModel = EtModel::createModelFromFile(etDevice, "assets/scenes/TopHole.obj");
        auto OnTopDupe3B = EtGameObject::createGameObject();
        OnTopDupe3B.model = etModel;
        OnTopDupe3B.transform.rotation = {0.f, 1.5708f, 0.f};
        OnTopDupe3B.transform.translation = {30.f, 7.0f, -7.f};
        OnTopDupe3B.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back((std::move(OnTopDupe3B)));

        etModel = EtModel::createModelFromFile(etDevice, "assets/scenes/TopHole.obj");
        auto OnTopDupe4B = EtGameObject::createGameObject();
        OnTopDupe4B.model = etModel;
        OnTopDupe4B.transform.rotation = {0.f, 1.5708f, 0.f};
        OnTopDupe4B.transform.translation = {-10.f, 7.0f, -7.f};
        OnTopDupe4B.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back((std::move(OnTopDupe4B)));

        etModel = EtModel::createModelFromFile(etDevice, "assets/scenes/TopHole.obj");
        auto OnTopDupe5B = EtGameObject::createGameObject();
        OnTopDupe5B.model = etModel;
        OnTopDupe5B.transform.rotation = {0.f, 1.5708f, 0.f};
        OnTopDupe5B.transform.translation = {-20.f, 7.0f, -7.f};
        OnTopDupe5B.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back((std::move(OnTopDupe5B)));

        etModel = EtModel::createModelFromFile(etDevice, "assets/scenes/UnderHoles.obj");
        auto Under = EtGameObject::createGameObject();
        Under.model = etModel;
        Under.transform.rotation = {0.f, 1.5708f, 0.f};
        Under.transform.translation = {0.f, 7.0f, 3.f};
        Under.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back((std::move(Under)));

        etModel = EtModel::createModelFromFile(etDevice, "assets/scenes/UnderHoles.obj");
        auto UnderDupe1 = EtGameObject::createGameObject();
        UnderDupe1.model = etModel;
        UnderDupe1.transform.rotation = {0.f, 1.5708f, 0.f};
        UnderDupe1.transform.translation = {10.f, 7.0f, 3.f};
        UnderDupe1.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back((std::move(UnderDupe1)));
        
        etModel = EtModel::createModelFromFile(etDevice, "assets/scenes/UnderHoles.obj");
        auto UnderDupe2 = EtGameObject::createGameObject();
        UnderDupe2.model = etModel;
        UnderDupe2.transform.rotation = {0.f, 1.5708f, 0.f};
        UnderDupe2.transform.translation = {20.f, 7.0f, 3.f};
        UnderDupe2.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back((std::move(UnderDupe2)));

        etModel = EtModel::createModelFromFile(etDevice, "assets/scenes/UnderHoles.obj");
        auto UnderDupe3 = EtGameObject::createGameObject();
        UnderDupe3.model = etModel;
        UnderDupe3.transform.rotation = {0.f, 1.5708f, 0.f};
        UnderDupe3.transform.translation = {30.f, 7.0f, 3.f};
        UnderDupe3.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back((std::move(UnderDupe3)));

        etModel = EtModel::createModelFromFile(etDevice, "assets/scenes/UnderHoles.obj");
        auto UnderDupe4 = EtGameObject::createGameObject();
        UnderDupe4.model = etModel;
        UnderDupe4.transform.rotation = {0.f, 1.5708f, 0.f};
        UnderDupe4.transform.translation = {-10.f, 7.0f, 3.f};
        UnderDupe4.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back((std::move(UnderDupe4)));

        etModel = EtModel::createModelFromFile(etDevice, "assets/scenes/UnderHoles.obj");
        auto UnderDupe5 = EtGameObject::createGameObject();
        UnderDupe5.model = etModel;
        UnderDupe5.transform.rotation = {0.f, 1.5708f, 0.f};
        UnderDupe5.transform.translation = {-20.f, 7.0f, 3.f};
        UnderDupe5.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back((std::move(UnderDupe5)));

        etModel = EtModel::createModelFromFile(etDevice, "assets/scenes/UnderHoles.obj");
        auto UnderDupeT = EtGameObject::createGameObject();
        UnderDupeT.model = etModel;
        UnderDupeT.transform.rotation = {0.f, 1.5708f, 0.f};
        UnderDupeT.transform.translation = {0.f, 7.0f, 33.f};
        UnderDupeT.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back((std::move(UnderDupeT)));

        etModel = EtModel::createModelFromFile(etDevice, "assets/scenes/UnderHoles.obj");
        auto UnderDupe1T = EtGameObject::createGameObject();
        UnderDupe1T.model = etModel;
        UnderDupe1T.transform.rotation = {0.f, 1.5708f, 0.f};
        UnderDupe1T.transform.translation = {10.f, 7.0f, 33.f};
        UnderDupe1T.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back((std::move(UnderDupe1T)));
        
        etModel = EtModel::createModelFromFile(etDevice, "assets/scenes/UnderHoles.obj");
        auto UnderDupe2T = EtGameObject::createGameObject();
        UnderDupe2T.model = etModel;
        UnderDupe2T.transform.rotation = {0.f, 1.5708f, 0.f};
        UnderDupe2T.transform.translation = {20.f, 7.0f, 33.f};
        UnderDupe2T.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back((std::move(UnderDupe2T)));

        etModel = EtModel::createModelFromFile(etDevice, "assets/scenes/UnderHoles.obj");
        auto UnderDupe3T = EtGameObject::createGameObject();
        UnderDupe3T.model = etModel;
        UnderDupe3T.transform.rotation = {0.f, 1.5708f, 0.f};
        UnderDupe3T.transform.translation = {30.f, 7.0f, 33.f};
        UnderDupe3T.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back((std::move(UnderDupe3T)));

        etModel = EtModel::createModelFromFile(etDevice, "assets/scenes/UnderHoles.obj");
        auto UnderDupe4T = EtGameObject::createGameObject();
        UnderDupe4T.model = etModel;
        UnderDupe4T.transform.rotation = {0.f, 1.5708f, 0.f};
        UnderDupe4T.transform.translation = {-10.f, 7.0f, 33.f};
        UnderDupe4T.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back((std::move(UnderDupe4T)));

        etModel = EtModel::createModelFromFile(etDevice, "assets/scenes/UnderHoles.obj");
        auto UnderDupe5T = EtGameObject::createGameObject();
        UnderDupe5T.model = etModel;
        UnderDupe5T.transform.rotation = {0.f, 1.5708f, 0.f};
        UnderDupe5T.transform.translation = {-20.f, 7.0f, 33.f};
        UnderDupe5T.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back((std::move(UnderDupe5T)));

        etModel = EtModel::createModelFromFile(etDevice, "assets/et/EtIdle.obj");
        auto et = EtGameObject::createGameObject();
        et.model = etModel;
        et.transform.translation = {0.0f, 6.7f, 13.f};
        et.transform.scale = {0.25f, 0.25f, 0.25f};
        gameObjects.push_back((std::move(et)));
    }
}