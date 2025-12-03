#pragma once
#include <vector>
#include <cstddef>
#include "student/VKBuffer.hpp"

namespace student {

    struct UBOData {
        vector<VulkanBuffer> bufferData; 
        int binding;           
    };

    UBOData createVulkanUniformBufferData(  VulkanInitData &vkInitData,
                                            vk::DeviceSize size,                                            
                                            int maxFramesInFlights,
                                            int binding = 0);
                                            
    void cleanupVulkanUniformBufferData(VulkanInitData &vkInitData,
                                        UBOData &uboData);

    vector<vk::DescriptorSetLayout> createDescriptorSetLayouts(
        VulkanInitData &vkInitData,
        const vector<vk::DescriptorSetLayoutBinding> &allBindings);  
        
    vk::DescriptorPool createDescriptorPool(
        VulkanInitData &vkInitData, 
        int uboCnt, 
        int numberFramesInFlight);

    vector<vk::DescriptorSet> createDescriptorSets(
        VulkanInitData &vkInitData, 
        const vector<vk::DescriptorSetLayout> &baseDescriptorSetLayouts,
        vk::DescriptorPool &descriptorPool,
        int numberFramesInFlight,
        const vector<UBOData> &allUBOData
    );
}