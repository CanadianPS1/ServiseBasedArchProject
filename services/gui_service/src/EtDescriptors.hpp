#pragma once
#include "EtDevice.hpp"
// std
#include <memory>
#include <unordered_map>
#include <vector>
namespace et{
    class EtDescriptorSetLayout{
        public:
            class Builder{
                public:
                    Builder(EtDevice &etDevice) : etDevice{etDevice}{}
                    Builder &addBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t count = 1);
                    std::unique_ptr<EtDescriptorSetLayout> build() const;
                private:
                    EtDevice &etDevice;
                std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
            };
            EtDescriptorSetLayout(EtDevice &etDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
            ~EtDescriptorSetLayout();
            EtDescriptorSetLayout(const EtDescriptorSetLayout &) = delete;
            EtDescriptorSetLayout &operator=(const EtDescriptorSetLayout &) = delete;
            VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }
        private:
            EtDevice &etDevice;
            VkDescriptorSetLayout descriptorSetLayout;
            std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;
        friend class EtDescriptorWriter;
    };
    class EtDescriptorPool{
        public:
            class Builder{
                public:
                    Builder(EtDevice &etDevice) : etDevice{etDevice}{}
                    Builder &addPoolSize(VkDescriptorType descriptorType, uint32_t count);
                    Builder &setPoolFlags(VkDescriptorPoolCreateFlags flags);
                    Builder &setMaxSets(uint32_t count);
                    std::unique_ptr<EtDescriptorPool> build() const;
                private:
                    EtDevice &etDevice;
                    std::vector<VkDescriptorPoolSize> poolSizes{};
                    uint32_t maxSets = 1000;
                    VkDescriptorPoolCreateFlags poolFlags = 0;
            };
            EtDescriptorPool(EtDevice &etDevice, uint32_t maxSets, VkDescriptorPoolCreateFlags poolFlags, const std::vector<VkDescriptorPoolSize> &poolSizes);
            ~EtDescriptorPool();
            EtDescriptorPool(const EtDescriptorPool &) = delete;
            EtDescriptorPool &operator=(const EtDescriptorPool &) = delete;
            bool allocateDescriptor(const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet &descriptor) const;
            void freeDescriptors(std::vector<VkDescriptorSet> &descriptors) const;
            void resetPool();
        private:
            EtDevice &etDevice;
            VkDescriptorPool descriptorPool;
        friend class EtDescriptorWriter;
    };
    class EtDescriptorWriter{
        public:
            EtDescriptorWriter(EtDescriptorSetLayout &setLayout, EtDescriptorPool &pool);
            EtDescriptorWriter &writeBuffer(uint32_t binding, VkDescriptorBufferInfo *bufferInfo);
            EtDescriptorWriter &writeImage(uint32_t binding, VkDescriptorImageInfo *imageInfo);
            bool build(VkDescriptorSet &set);
            void overwrite(VkDescriptorSet &set);
        private:
            EtDescriptorSetLayout &setLayout;
            EtDescriptorPool &pool;
            std::vector<VkWriteDescriptorSet> writes;
    };  
}