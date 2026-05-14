#pragma once
#include "EtWindow.hpp"
#include "EtDevice.hpp"
#include "EtSwapChain.hpp"
#include <memory>
#include <vector>
#include <cassert>
namespace et{
    class EtRenderer{
        public:
            EtRenderer(EtWindow &window, EtDevice &device);
            ~EtRenderer();
            EtRenderer(const EtRenderer &) = delete;
            EtRenderer &operator=(const EtRenderer &) = delete;
            VkCommandBuffer beginFrame();
            void endFrame();
            void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
            void endSwapChainRenderPass(VkCommandBuffer commandBuffer);
            VkRenderPass getSwapChainRenderPass() const {return etSwapChain->getRenderPass();}
            bool isFrameInProgress() const {return isFrameStarted;}
            VkCommandBuffer getCurrentCommandBuffer() const {
                assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
                return commandBuffers[currentFrameIndex];
            }
            int getFrameIndex() const{
                assert(isFrameStarted && "Cannot get frame index when frame not in progress");
                return currentFrameIndex;
            }
        private:
            void createCommandBuffers();
            void freeCommandBuffers();
            void recreateSwapChain();
            EtWindow& etWindow;
            EtDevice& etDevice;
            std::unique_ptr<EtSwapChain> etSwapChain;
            std::vector<VkCommandBuffer> commandBuffers;
            uint32_t currentImageIndex;
            int currentFrameIndex{0};
            bool isFrameStarted = false;
    };
}