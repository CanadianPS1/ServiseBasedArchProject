#include "EtMain.hpp"
#include "EtSwapChain.hpp"
#include <GLFW/glfw3.h>
#include <memory>
#include <stdexcept>
#include <array>
#include <vulkan/vulkan_core.h>
#include <stdexcept>
namespace et{
    EtMain::EtMain(){
        createPipelineLayout();
        createPipeline();
        createCommandBuffers();
    }
    EtMain::~EtMain(){vkDestroyPipelineLayout(etDevice.device(), pipelineLayout, nullptr);}
    void EtMain::run(){
        while(!etWindow.shouldClose()){
            glfwPollEvents();
            drawFrame();
        }
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
        auto pipelineConfig = EtPipeline::defultPipelineConfigInfo(etSwapChain.width(), etSwapChain.height());
        pipelineConfig.renderPass = etSwapChain.getRenderPass();
        pipelineConfig.pipelineLayout = pipelineLayout;
        etPipeline = std::make_unique<EtPipeline>(
            etDevice,
            "shaders/simple_shader.vert.spv",
            "shaders/simple_shader.frag.spv",
            pipelineConfig
        );
    }
    void EtMain::createCommandBuffers(){
        commandBuffers.resize(etSwapChain.imageCount());
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = etDevice.getCommandPool();
        allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
        if(vkAllocateCommandBuffers(etDevice.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate command buffers");
        for(int i = 0; i < commandBuffers.size(); i++){
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            if(vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS)
                throw std::runtime_error("failed to begin recording command buffer");
            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = etSwapChain.getRenderPass();
            renderPassInfo.framebuffer = etSwapChain.getFrameBuffer(i);
            renderPassInfo.renderArea.offset = {0,0};
            renderPassInfo.renderArea.extent = etSwapChain.getSwapChainExtent();

            std::array<VkClearValue, 2> clearValues{};
            clearValues[0].color = {0.1f, 0.1f, 0.1f, 1.0f};
            clearValues[1].depthStencil = {1.0f, 0};
            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();
            vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            etPipeline->bind(commandBuffers[i]);
            vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);
            vkCmdEndRenderPass(commandBuffers[i]);
            if(vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
                throw std::runtime_error("failed to crecord command buffer");
        }
    }   
    void EtMain::drawFrame(){
        uint32_t imageIndex;
        auto result = etSwapChain.acquireNextImage(&imageIndex);
        if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            throw std::runtime_error("failed to aquire swap chain image");
        result = etSwapChain.submitCommandBuffers(&commandBuffers[imageIndex],&imageIndex);
        // if(result != VK_SUCCESS)
        //     throw std::runtime_error("failed to present swap chain image");
    }
}