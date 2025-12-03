#include "student/VKUniform.hpp"

namespace student {

    UBOData createVulkanUniformBufferData(  VulkanInitData &vkInitData,
                                            vk::DeviceSize size,                                            
                                            int maxFramesInFlights,
                                            int binding) {        
        UBOData data;
        data.binding = binding;
        data.bufferData.resize(maxFramesInFlights);         

        for (unsigned int i = 0; i < maxFramesInFlights; i++) {
            data.bufferData[i] = createVulkanBuffer(vkInitData,
                                                    size,
                                                    vk::BufferUsageFlagBits::eUniformBuffer,
                                                    createVMAHostVisibleInfo());
        }

        return data;
    }

    void cleanupVulkanUniformBufferData(VulkanInitData &vkInitData,
                                        UBOData &uboData) {

        for(unsigned int i = 0; i < uboData.bufferData.size(); i++) {
            cleanupVulkanBuffer(vkInitData, uboData.bufferData[i]);
        }
        uboData.bufferData.clear();        
    }

    vector<vk::DescriptorSetLayout> createDescriptorSetLayouts(
        VulkanInitData &vkInitData,
        const vector<vk::DescriptorSetLayoutBinding> &allBindings) {

        vk::DescriptorSetLayout descriptorSetLayout = vkInitData.device.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo({}, allBindings));
        return {descriptorSetLayout};
    }

    vk::DescriptorPool createDescriptorPool(
        VulkanInitData &vkInitData, 
        int uboCnt, 
        int numberFramesInFlight) {

        vector<vk::DescriptorPoolSize> poolSizes = {
            vk::DescriptorPoolSize(
                vk::DescriptorType::eUniformBuffer, 
                uboCnt*numberFramesInFlight),
        };
        
        vk::DescriptorPool descriptorPool = vkInitData.device.createDescriptorPool(
            vk::DescriptorPoolCreateInfo()
                .setPoolSizes(poolSizes)                    
                .setMaxSets(numberFramesInFlight));

        return descriptorPool;
    }

    vector<vk::DescriptorSet> createDescriptorSets(
        VulkanInitData &vkInitData, 
        const vector<vk::DescriptorSetLayout> &baseDescriptorSetLayouts,
        vk::DescriptorPool &descriptorPool,
        int numberFramesInFlight,
        const vector<UBOData> &allUBOData
    ) {
        vector<vk::DescriptorSetLayout> layouts;
        for(unsigned int i = 0; i < numberFramesInFlight; i++) {
            layouts.push_back(baseDescriptorSetLayouts.at(0));
        }

        vector<vk::DescriptorSet> descriptorSets = vkInitData.device.allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo()
		        .setDescriptorPool(descriptorPool)
		        .setDescriptorSetCount(numberFramesInFlight)
		        .setSetLayouts(layouts)
        );

        for(int i = 0; i < numberFramesInFlight; i++) {
            vector<vk::DescriptorBufferInfo> infos;
            infos.resize(allUBOData.size());
            
            for(int j = 0; j < allUBOData.size(); j++) {
                infos[j] = vk::DescriptorBufferInfo()
                    .setBuffer(allUBOData[j].bufferData[i].buffer)
                    .setOffset(0)
                    .setRange(allUBOData[j].bufferData[i].size);
            }

            vector<vk::WriteDescriptorSet> writes;
            writes.resize(allUBOData.size());            

            for(int j = 0; j < allUBOData.size(); j++) {
                writes[j] = vk::WriteDescriptorSet()
                    .setDstSet(descriptorSets[i])
                    .setDstBinding(allUBOData[j].binding)
                    .setDstArrayElement(0)
                    .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                    .setDescriptorCount(1)
                    .setBufferInfo(infos[j]);
            }

            vkInitData.device.updateDescriptorSets(writes, {}); 
        }

        return descriptorSets;    
    }
}