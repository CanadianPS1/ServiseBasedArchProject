#pragma once
#include "EtPipeline.hpp"
#include "EtDevice.hpp"
#include "EtCamera.hpp"
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>
#include "EtGameObject.hpp"
namespace et{
    class SimpleRenderSystem{
        public:
            SimpleRenderSystem(EtDevice& device, VkRenderPass renderPass);
            ~SimpleRenderSystem();
            SimpleRenderSystem(const SimpleRenderSystem &) = delete;
            SimpleRenderSystem &operator=(const SimpleRenderSystem &) = delete;
            void renderGameObjects(VkCommandBuffer commandBuffer, std::vector<EtGameObject> &gameObjects, const EtCamera &camera);
        private:
            void createPipelineLayout();
            void createPipeline(VkRenderPass renderPass);
            EtDevice& etDevice;
            std::unique_ptr<EtPipeline> etPipeline;
            VkPipelineLayout pipelineLayout;
    };
}