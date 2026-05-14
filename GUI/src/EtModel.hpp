#pragma once
#include "EtDevice.hpp"
#include <vulkan/vulkan_core.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <vector>
namespace et{
    class EtModel{
        public:
            struct Vertex{
                glm::vec2 position;
                glm::vec3 color;
                static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
                static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
            };
            EtModel(EtDevice& device, const std::vector<Vertex> &vertices);
            ~EtModel();
            EtModel(const EtModel &) = delete;
            EtModel &operator=(const EtModel &) = delete;
            void bind(VkCommandBuffer commandBuffer);
            void draw(VkCommandBuffer commandBuffer);
        private:
            void createVertexBuffers(const std::vector<Vertex> &vertices);
            EtDevice& etDevice;
            VkBuffer vertexBuffer;
            VkDeviceMemory vertexBufferMemory;
            uint32_t vertexCount;
    };
}