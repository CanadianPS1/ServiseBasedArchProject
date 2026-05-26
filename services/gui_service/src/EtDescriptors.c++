#include "EtDescriptors.hpp"
// std
#include <cassert>
#include <stdexcept>
namespace et{
    EtDescriptorSetLayout::Builder &EtDescriptorSetLayout::Builder::addBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t count){
        assert(bindings.count(binding) == 0 && "Binding already in use");
        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = binding;
        layoutBinding.descriptorType = descriptorType;
        layoutBinding.descriptorCount = count;
        layoutBinding.stageFlags = stageFlags;
        bindings[binding] = layoutBinding;
        return *this;
    }
    std::unique_ptr<EtDescriptorSetLayout> EtDescriptorSetLayout::Builder::build() const {return std::make_unique<EtDescriptorSetLayout>(etDevice, bindings);}
    EtDescriptorSetLayout::EtDescriptorSetLayout(EtDevice &etDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings) : etDevice{etDevice}, bindings{bindings}{
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
        for(auto kv : bindings) setLayoutBindings.push_back(kv.second);
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
        descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
        descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();
        if(vkCreateDescriptorSetLayout(etDevice.device(), &descriptorSetLayoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
            throw std::runtime_error("failed to create descriptor set layout!");
    }
    EtDescriptorSetLayout::~EtDescriptorSetLayout(){vkDestroyDescriptorSetLayout(etDevice.device(), descriptorSetLayout, nullptr);}
    EtDescriptorPool::Builder &EtDescriptorPool::Builder::addPoolSize(VkDescriptorType descriptorType, uint32_t count){
        poolSizes.push_back({descriptorType, count});
        return *this;
    }
    EtDescriptorPool::Builder &EtDescriptorPool::Builder::setPoolFlags(VkDescriptorPoolCreateFlags flags){
        poolFlags = flags;
        return *this;
    }
    EtDescriptorPool::Builder &EtDescriptorPool::Builder::setMaxSets(uint32_t count){
        maxSets = count;
        return *this;
    }
    std::unique_ptr<EtDescriptorPool> EtDescriptorPool::Builder::build() const {return std::make_unique<EtDescriptorPool>(etDevice, maxSets, poolFlags, poolSizes);}
    EtDescriptorPool::EtDescriptorPool(EtDevice &etDevice, uint32_t maxSets, VkDescriptorPoolCreateFlags poolFlags, const std::vector<VkDescriptorPoolSize> &poolSizes) : etDevice{etDevice}{
        VkDescriptorPoolCreateInfo descriptorPoolInfo{};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        descriptorPoolInfo.pPoolSizes = poolSizes.data();
        descriptorPoolInfo.maxSets = maxSets;
        descriptorPoolInfo.flags = poolFlags;
        if(vkCreateDescriptorPool(etDevice.device(), &descriptorPoolInfo, nullptr, &descriptorPool) != VK_SUCCESS) 
            throw std::runtime_error("failed to create descriptor pool!");
    } 
    EtDescriptorPool::~EtDescriptorPool(){vkDestroyDescriptorPool(etDevice.device(), descriptorPool, nullptr);}
    bool EtDescriptorPool::allocateDescriptor(const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet &descriptor) const {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.pSetLayouts = &descriptorSetLayout;
        allocInfo.descriptorSetCount = 1;
        if(vkAllocateDescriptorSets(etDevice.device(), &allocInfo, &descriptor) != VK_SUCCESS) return false;
        return true;
    }
    void EtDescriptorPool::freeDescriptors(std::vector<VkDescriptorSet> &descriptors) const {vkFreeDescriptorSets(etDevice.device(), descriptorPool, static_cast<uint32_t>(descriptors.size()), descriptors.data());}
    void EtDescriptorPool::resetPool(){vkResetDescriptorPool(etDevice.device(), descriptorPool, 0);}
    EtDescriptorWriter::EtDescriptorWriter(EtDescriptorSetLayout &setLayout, EtDescriptorPool &pool) : setLayout{setLayout}, pool{pool} {}
    EtDescriptorWriter &EtDescriptorWriter::writeBuffer(uint32_t binding, VkDescriptorBufferInfo *bufferInfo){
        assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");
        auto &bindingDescription = setLayout.bindings[binding];
        assert(bindingDescription.descriptorCount == 1 && "Binding single descriptor info, but binding expects multiple");
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType = bindingDescription.descriptorType;
        write.dstBinding = binding;
        write.pBufferInfo = bufferInfo;
        write.descriptorCount = 1;
        writes.push_back(write);
        return *this;
    }
    EtDescriptorWriter &EtDescriptorWriter::writeImage(uint32_t binding, VkDescriptorImageInfo *imageInfo){
        assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");
        auto &bindingDescription = setLayout.bindings[binding];
        assert(bindingDescription.descriptorCount == 1 && "Binding single descriptor info, but binding expects multiple");
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType = bindingDescription.descriptorType;
        write.dstBinding = binding;
        write.pImageInfo = imageInfo;
        write.descriptorCount = 1;
        writes.push_back(write);
        return *this;
    }
    bool EtDescriptorWriter::build(VkDescriptorSet &set){
        bool success = pool.allocateDescriptor(setLayout.getDescriptorSetLayout(), set);
        if(!success) return false;
        overwrite(set);
        return true;
    }
    void EtDescriptorWriter::overwrite(VkDescriptorSet &set){
        for(auto &write : writes) write.dstSet = set;
        vkUpdateDescriptorSets(pool.etDevice.device(), writes.size(), writes.data(), 0, nullptr);
    }
}