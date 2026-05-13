#pragma once
#include "EtDevice.hpp"
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>
namespace et{
    struct PipelineConfigInfo{
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
        VkViewport viewport;
        VkRect2D scissor;
        VkPipelineRasterizationStateCreateInfo rasterizationInfo;
        VkPipelineMultisampleStateCreateInfo multisampleInfo;
        VkPipelineColorBlendAttachmentState colorBlendAttachment;
        VkPipelineColorBlendStateCreateInfo colorBlendInfo;
        VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
        VkPipelineLayout pipelineLayout = nullptr;
        VkRenderPass renderPass = nullptr;
        uint32_t subpass = 0;
    };
    class EtPipeline{
        public:
        EtPipeline(EtDevice &device, const std::string& vertFilepath, const std::string& fragFilepath, const PipelineConfigInfo& configInfo);
        ~EtPipeline();
        EtPipeline(const EtPipeline&) = delete;
        void operator=(const EtPipeline&) = delete;
        void bind(VkCommandBuffer commandBuffer);
        static PipelineConfigInfo defultPipelineConfigInfo(uint32_t width, uint32_t height);
        private:
        static std::vector<char> readFile(const std::string& filepath);
        void createGraphicsPipeline(const std::string& vertFilepath, const std::string& fragFilepath, const PipelineConfigInfo& configInfo);
        void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);
        EtDevice& etDevice;
        VkPipeline graphicsPipeline;
        VkShaderModule vertShaderModule;
        VkShaderModule fragShaderModule;
    };
}