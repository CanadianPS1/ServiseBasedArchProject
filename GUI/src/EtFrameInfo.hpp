#pragma once
#include "EtCamera.hpp"
#include <vulkan/vulkan.h>
namespace et{
    struct FrameInfo{
        int frameIndex;
        float frameTime;
        VkCommandBuffer commandBuffer;
        EtCamera &camera;
        VkDescriptorSet globalDescriptorSet;
    };
}