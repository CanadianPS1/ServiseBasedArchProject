#pragma once
#include "EtDevice.hpp"
#include "EtBuffer.hpp"
#include <vulkan/vulkan_core.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <memory>
#include <vector>
namespace et{
    class EtModel{
        public:
            struct Vertex{
                glm::vec3 position{};
                glm::vec3 color{};
                glm::vec3 normal{};
                glm::vec2 uv{};
                static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
                static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
                std::vector<Vertex> vertices{};
                bool operator==(const Vertex &other) const {
                    return position == other.position && color == other.color && normal == other.normal && uv == other.uv;
                }
            };
            struct Builder{
                std::vector<Vertex> vertices{};
                std::vector<uint32_t> indices{};
                void loadModel(const std::string &filepath);
            };
            EtModel(EtDevice& device, const EtModel::Builder &builder);
            ~EtModel();
            EtModel(const EtModel &) = delete;
            EtModel &operator=(const EtModel &) = delete;
            void bind(VkCommandBuffer commandBuffer);
            void draw(VkCommandBuffer commandBuffer);
            static std::unique_ptr<EtModel> createModelFromFile(EtDevice &device, const std::string &filepath);
        private:
            void createVertexBuffers(const std::vector<Vertex> &vertices);
            void createIndexBuffers(const std::vector<uint32_t> &indices);
            EtDevice& etDevice;
            std::unique_ptr<EtBuffer> vertexBuffer;
            uint32_t vertexCount;
            bool hasIndexBuffer = false;
            std::unique_ptr<EtBuffer> indexBuffer;
            uint32_t indexCount;
    };
}