#include "EtMain.hpp"
#include "EtPipeline.hpp"
#include "EtSwapChain.hpp"
#include <GLFW/glfw3.h>
#include <memory>
#include <stdexcept>
#include <array>
#include <vulkan/vulkan_core.h>
#include <stdexcept>
namespace et{
    EtMain::EtMain(){
        loadModels();
        createPipelineLayout();
        recreateSwapChain();
        createCommandBuffers();
    }
    EtMain::~EtMain(){vkDestroyPipelineLayout(etDevice.device(), pipelineLayout, nullptr);}
    void EtMain::run(){
        while(!etWindow.shouldClose()){
            glfwPollEvents();
            drawFrame();
        }
        vkDeviceWaitIdle(etDevice.device());
    }
    void EtMain::createPipelineLayout(){
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;
        if(vkCreatePipelineLayout(etDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
            throw std::runtime_error("failed to create pipeline layout");
    }
    void EtMain::createPipeline(){
        assert(etSwapChain != nullptr && "Cannot create pipeline before swap chain");
        assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");
        PipelineConfigInfo pipelineConfig{};
        EtPipeline::defultPipelineConfigInfo(pipelineConfig);
        pipelineConfig.renderPass = etSwapChain->getRenderPass();
        pipelineConfig.pipelineLayout = pipelineLayout;
        etPipeline = std::make_unique<EtPipeline>(
            etDevice,
            "shaders/simple_shader.vert.spv",
            "shaders/simple_shader.frag.spv",
            pipelineConfig
        );
    }
    void EtMain::createCommandBuffers(){
        commandBuffers.resize(etSwapChain->imageCount());
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = etDevice.getCommandPool();
        allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
        if(vkAllocateCommandBuffers(etDevice.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate command buffers");
    }   
    void EtMain::drawFrame(){
        uint32_t imageIndex;
        auto result = etSwapChain->acquireNextImage(&imageIndex);
        if(result == VK_ERROR_OUT_OF_DATE_KHR){
            recreateSwapChain();
            return;
        }
        if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            throw std::runtime_error("failed to aquire swap chain image");
        recordCommandBuffer(imageIndex);
        result = etSwapChain->submitCommandBuffers(&commandBuffers[imageIndex],&imageIndex);
        if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || etWindow.wasWindowResized()){
            etWindow.resetWindowResizedFlag();
            recreateSwapChain();
            return;
        }
        if(result != VK_SUCCESS)
            throw std::runtime_error("failed to present swap chain image");
    }
    void EtMain::loadModels(){
        std::vector<EtModel::Vertex> vertices{
            {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
            {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
        };
        etModel = std::make_unique<EtModel>(etDevice, vertices);
    }
    void EtMain::recordCommandBuffer(int imageIndex){
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        if(vkBeginCommandBuffer(commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS)
            throw std::runtime_error("failed to begin recording command buffer");
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = etSwapChain->getRenderPass();
        renderPassInfo.framebuffer = etSwapChain->getFrameBuffer(imageIndex);
        renderPassInfo.renderArea.offset = {0,0};
        renderPassInfo.renderArea.extent = etSwapChain->getSwapChainExtent();

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {0.1f, 0.1f, 0.1f, 1.0f};
        clearValues[1].depthStencil = {1.0f, 0};
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();
        vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(etSwapChain->getSwapChainExtent().width);
        viewport.height = static_cast<float>(etSwapChain->getSwapChainExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        VkRect2D scissor{{0,0}, etSwapChain->getSwapChainExtent()};
        vkCmdSetViewport(commandBuffers[imageIndex], 0, 1, &viewport);
        vkCmdSetScissor(commandBuffers[imageIndex], 0, 1, &scissor);
        etPipeline->bind(commandBuffers[imageIndex]);
        etModel->bind(commandBuffers[imageIndex]);
        etModel->draw(commandBuffers[imageIndex]);
        vkCmdEndRenderPass(commandBuffers[imageIndex]);
        if(vkEndCommandBuffer(commandBuffers[imageIndex]) != VK_SUCCESS)
            throw std::runtime_error("failed to crecord command buffer");
    }
    void EtMain::recreateSwapChain(){
        auto extent = etWindow.getExtent();
        while(extent.width == 0 || extent.height == 0){
            extent = etWindow.getExtent();
            glfwWaitEvents();
        }
        vkDeviceWaitIdle(etDevice.device());
        if(etSwapChain == nullptr) etSwapChain = std::make_unique<EtSwapChain>(etDevice, extent);
        else{ 
            etSwapChain = std::make_unique<EtSwapChain>(etDevice, extent, std::move(etSwapChain));
            if(etSwapChain->imageCount() != commandBuffers.size()){
                freeCommandBuffers();
                createCommandBuffers();
            }
        }
        createPipeline();
    }
    void EtMain::freeCommandBuffers(){
        vkFreeCommandBuffers(etDevice.device(), etDevice.getCommandPool(), static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
        commandBuffers.clear();
    }
}