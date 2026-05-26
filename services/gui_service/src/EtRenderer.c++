#include "EtRenderer.hpp"
#include <cassert>
#include <stdexcept>
#include <array>
#include <stdexcept>
namespace et{
    EtRenderer::EtRenderer(EtWindow &window, EtDevice& device) : etWindow{window}, etDevice{device}{
        recreateSwapChain();
        createCommandBuffers();
    }
    EtRenderer::~EtRenderer(){freeCommandBuffers();}
    void EtRenderer::recreateSwapChain(){
        auto extent = etWindow.getExtent();
        while(extent.width == 0 || extent.height == 0){
            extent = etWindow.getExtent();
            glfwWaitEvents();
        }
        vkDeviceWaitIdle(etDevice.device());
        if(etSwapChain == nullptr) etSwapChain = std::make_unique<EtSwapChain>(etDevice, extent);
        else{ 
            std::shared_ptr<EtSwapChain> oldSwapChain = std::move(etSwapChain);
            etSwapChain = std::make_unique<EtSwapChain>(etDevice, extent, oldSwapChain);
            if(!oldSwapChain->compareSwapFormats(*etSwapChain.get())) 
                throw std::runtime_error("Swap chain image (or depth) format has changed");
        }
    }
    void EtRenderer::createCommandBuffers(){
        commandBuffers.resize(EtSwapChain::MAX_FRAMES_IN_FLIGHT);
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = etDevice.getCommandPool();
        allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
        if(vkAllocateCommandBuffers(etDevice.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate command buffers");
    }   
    void EtRenderer::freeCommandBuffers(){
        vkFreeCommandBuffers(etDevice.device(), etDevice.getCommandPool(), static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
        commandBuffers.clear();
    }
    VkCommandBuffer EtRenderer::beginFrame(){
        assert(!isFrameStarted && "cannot call beginframe while already in progress");
        auto result = etSwapChain->acquireNextImage(&currentImageIndex);
        if(result == VK_ERROR_OUT_OF_DATE_KHR){
            recreateSwapChain();
            return nullptr;
        }
        if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            throw std::runtime_error("failed to aquire swap chain image");
        isFrameStarted = true;
        auto commandBuffer = getCurrentCommandBuffer();
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        if(vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
            throw std::runtime_error("failed to begin recording command buffer");
        return commandBuffer;
    }
    void EtRenderer::endFrame(){
        assert(isFrameStarted && "cannot call end frame while frame is not in progress");
        auto commandBuffer = getCurrentCommandBuffer();
        if(vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
            throw std::runtime_error("failed to crecord command buffer");
        auto result = etSwapChain->submitCommandBuffers(&commandBuffer,&currentImageIndex);
        if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || etWindow.wasWindowResized()){
            etWindow.resetWindowResizedFlag();
            recreateSwapChain();
        }
        // if(result != VK_SUCCESS)
        //     throw std::runtime_error("failed to present swap chain image");
        isFrameStarted = false;
        currentFrameIndex = (currentFrameIndex + 1) % EtSwapChain::MAX_FRAMES_IN_FLIGHT;
    }
    void EtRenderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer){
        assert(isFrameStarted && "cannot call beginSwapChainRenderPass while frame is not in progress");
        assert(commandBuffer == getCurrentCommandBuffer() && "cant begin render pass on command buffer from a diffrent frame");
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = etSwapChain->getRenderPass();
        renderPassInfo.framebuffer = etSwapChain->getFrameBuffer(currentImageIndex);
        renderPassInfo.renderArea.offset = {0,0};
        renderPassInfo.renderArea.extent = etSwapChain->getSwapChainExtent();

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {0.01f, 0.01f, 0.01f, 1.0f};
        clearValues[1].depthStencil = {1.0f, 0};
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(etSwapChain->getSwapChainExtent().width);
        viewport.height = static_cast<float>(etSwapChain->getSwapChainExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        VkRect2D scissor{{0,0}, etSwapChain->getSwapChainExtent()};
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    }
    void EtRenderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer){
        assert(isFrameStarted && "cannot call endSwapChainRenderPass while frame is not in progress");
        assert(commandBuffer == getCurrentCommandBuffer() && "cant end render pass on command buffer from a diffrent frame");
        vkCmdEndRenderPass(commandBuffer);
    }
}