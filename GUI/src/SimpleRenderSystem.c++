#include "SimpleRenderSystem.hpp"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include "SimpleRenderSystem.hpp"
#include "EtPipeline.hpp"
#include <GLFW/glfw3.h>
#include <memory>
#include <stdexcept>
#include <vulkan/vulkan_core.h>
#include <stdexcept>
#include <glm/gtc/constants.hpp>
namespace et{
    struct SimplePushConstantData{
        glm::mat4 transform{1.f};
        alignas(16)glm::vec3 color;
    };
    SimpleRenderSystem::SimpleRenderSystem(EtDevice& device, VkRenderPass renderPass) : etDevice{device}{
        createPipelineLayout();
        createPipeline(renderPass);
    }
    SimpleRenderSystem::~SimpleRenderSystem(){vkDestroyPipelineLayout(etDevice.device(), pipelineLayout, nullptr);}
    void SimpleRenderSystem::createPipelineLayout(){
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(SimplePushConstantData);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        if(vkCreatePipelineLayout(etDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
            throw std::runtime_error("failed to create pipeline layout");
    }
    void SimpleRenderSystem::createPipeline(VkRenderPass renderPass){
        assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");
        PipelineConfigInfo pipelineConfig{};
        EtPipeline::defultPipelineConfigInfo(pipelineConfig);
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;
        etPipeline = std::make_unique<EtPipeline>(
            etDevice,
            "shaders/simple_shader.vert.spv",
            "shaders/simple_shader.frag.spv",
            pipelineConfig
        );
    }
    void SimpleRenderSystem::renderGameObjects(VkCommandBuffer commandBuffer, std::vector<EtGameObject> &gameObjects, const EtCamera &camera){
        etPipeline->bind(commandBuffer);
        auto projectionView = camera.getProjection() * camera.getView();
        for(auto& obj : gameObjects){
            SimplePushConstantData push{};
            push.color = obj.color;
            push.transform = projectionView * obj.transform.mat4();
            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0, sizeof(SimplePushConstantData), &push);
            obj.model->bind(commandBuffer);
            obj.model->draw(commandBuffer);
        }
    }
}