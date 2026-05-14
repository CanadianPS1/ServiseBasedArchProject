#pragma once
#include "EtWindow.hpp"
#include "EtPipeline.hpp"
#include "EtDevice.hpp"
#include "EtSwapChain.hpp"
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>
#include "EtModel.hpp"
namespace et{
    class EtMain{
        public:
            static constexpr int WIDTH = 1400;
            static constexpr int HEIGHT = 788;
            EtMain();
            ~EtMain();
            EtMain(const EtMain &) = delete;
            EtMain &operator=(const EtMain &) = delete;
            void run();
        private:
            void loadModels();
            void createPipelineLayout();
            void createPipeline();
            void createCommandBuffers();
            void freeCommandBuffers();
            void drawFrame();
            void recreateSwapChain();
            void recordCommandBuffer(int imageIndex);
            EtWindow etWindow{WIDTH, HEIGHT, "ET: Extra Terestial"};
            EtDevice etDevice{etWindow};
            std::unique_ptr<EtSwapChain> etSwapChain;
            std::unique_ptr<EtPipeline> etPipeline;
            VkPipelineLayout pipelineLayout;
            std::vector<VkCommandBuffer> commandBuffers;
            std::unique_ptr<EtModel> etModel;
    };
}