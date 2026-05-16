#pragma once
#include "EtPipeline.hpp"
#include "EtDevice.hpp"
#include "EtCamera.hpp"
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>
#include "EtGameObject.hpp"
#include "EtFrameInfo.hpp"
namespace et{
    class SimpleRenderSystem{
        public:
            SimpleRenderSystem(EtDevice& device, VkRenderPass renderPass);
            ~SimpleRenderSystem();
            SimpleRenderSystem(const SimpleRenderSystem &) = delete;
            SimpleRenderSystem &operator=(const SimpleRenderSystem &) = delete;
            void renderGameObjects(std::vector<EtGameObject> &gameObjects, FrameInfo& frameInfo);
        private:
            void createPipelineLayout();
            void createPipeline(VkRenderPass renderPass);
            EtDevice& etDevice;
            std::unique_ptr<EtPipeline> etPipeline;
            VkPipelineLayout pipelineLayout;
    };
}