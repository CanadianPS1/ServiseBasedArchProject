#include "EtBuffer.hpp"
#include <cassert>
#include <cstring>
namespace et{
    VkDeviceSize EtBuffer::getAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment) {
        if(minOffsetAlignment > 0) return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
        return instanceSize;
    }
    EtBuffer::EtBuffer(EtDevice &device, VkDeviceSize instanceSize, uint32_t instanceCount, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags,
    VkDeviceSize minOffsetAlignment) : etDevice{device}, instanceSize{instanceSize}, instanceCount{instanceCount}, usageFlags{usageFlags}, memoryPropertyFlags{memoryPropertyFlags}{
        alignmentSize = getAlignment(instanceSize, minOffsetAlignment);
        bufferSize = alignmentSize * instanceCount;
        device.createBuffer(bufferSize, usageFlags, memoryPropertyFlags, buffer, memory);
    }
    EtBuffer::~EtBuffer(){
        unmap();
        vkDestroyBuffer(etDevice.device(), buffer, nullptr);
        vkFreeMemory(etDevice.device(), memory, nullptr);
    }
    VkResult EtBuffer::map(VkDeviceSize size, VkDeviceSize offset){
        assert(buffer && memory && "Called map on buffer before create");
        return vkMapMemory(etDevice.device(), memory, offset, size, 0, &mapped);
    }
    void EtBuffer::unmap(){
        if(mapped){
            vkUnmapMemory(etDevice.device(), memory);
            mapped = nullptr;
        }
    }
    void EtBuffer::writeToBuffer(void *data, VkDeviceSize size, VkDeviceSize offset){
        assert(mapped && "Cannot copy to unmapped buffer");
        if(size == VK_WHOLE_SIZE) memcpy(mapped, data, bufferSize);
        else{
            char *memOffset = (char *)mapped;
            memOffset += offset;
            memcpy(memOffset, data, size);
        }
    }
    VkResult EtBuffer::flush(VkDeviceSize size, VkDeviceSize offset){
        VkMappedMemoryRange mappedRange = {};
        mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mappedRange.memory = memory;
        mappedRange.offset = offset;
        mappedRange.size = size;
        return vkFlushMappedMemoryRanges(etDevice.device(), 1, &mappedRange);
    }
    VkResult EtBuffer::invalidate(VkDeviceSize size, VkDeviceSize offset){
        VkMappedMemoryRange mappedRange = {};
        mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mappedRange.memory = memory;
        mappedRange.offset = offset;
        mappedRange.size = size;
        return vkInvalidateMappedMemoryRanges(etDevice.device(), 1, &mappedRange);
    }
    VkDescriptorBufferInfo EtBuffer::descriptorInfo(VkDeviceSize size, VkDeviceSize offset){return VkDescriptorBufferInfo{buffer, offset, size,};}
    void EtBuffer::writeToIndex(void *data, int index){writeToBuffer(data, instanceSize, index * alignmentSize);}
    VkResult EtBuffer::flushIndex(int index){return flush(alignmentSize, index * alignmentSize);}
    VkDescriptorBufferInfo EtBuffer::descriptorInfoForIndex(int index){return descriptorInfo(alignmentSize, index * alignmentSize);}
    VkResult EtBuffer::invalidateIndex(int index){return invalidate(alignmentSize, index * alignmentSize);}
}