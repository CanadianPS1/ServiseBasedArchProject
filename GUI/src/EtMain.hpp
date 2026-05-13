#pragma once
#include "EtWindow.hpp"
#include "EtPipeline.hpp"
#include "EtDevice.hpp"
#include "EtSwapChain.hpp"
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>
namespace et{
    class EtMain{
        public:
        static constexpr int WIDTH = 1400;
        static constexpr int HEIGHT = 800;
        EtMain();
        ~EtMain();
        EtMain(const EtMain &) = delete;
        EtMain &operator=(const EtMain &) = delete;
        void run();
        private:
        void createPipelineLayout();
        void createPipeline();
        void createCommandBuffers();
        void drawFrame();
        void recreateSwapChain();
        EtWindow etWindow{WIDTH, HEIGHT, "ET: Extra Terestial"};
        EtDevice etDevice{etWindow};
        EtSwapChain etSwapChain{etDevice, etWindow.getExtent()};
        std::unique_ptr<EtPipeline> etPipeline;
        VkPipelineLayout pipelineLayout;
        std::vector<VkCommandBuffer> commandBuffers;
    };
}