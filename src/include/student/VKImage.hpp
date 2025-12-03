#pragma once
#include <iostream>
#include <string>
#include "VKCore.hpp"
#include "student/VKSetup.hpp"

namespace student {
    struct VulkanImageTransition {
        vk::ImageMemoryBarrier barrier {};
        vk::PipelineStageFlags srcFlags {};
        vk::PipelineStageFlags dstFlags {};
    };

    enum VK_IMAGE_TRANSITION_TYPE {
        UNDEF_TO_COLOR,        
        COLOR_TO_PRESENT,
        UNDEF_TO_DEPTH   
    };

    VulkanImageTransition createVulkanImageTransition(vk::Image &image, 
                                                        VK_IMAGE_TRANSITION_TYPE type);
    void performVulkanImageTransition(  vk::CommandBuffer &commandBuffer, 
                                        VulkanImageTransition &transitionData); 

    struct VulkanImage {
        vk::Image image{};
        vk::ImageView view{};
        VmaAllocation allocation{};
        vk::Format format{};
        vk::Extent3D extent{};
        uint32_t mipLevels{1};
    };

    VulkanImage createVulkanImage(  VulkanInitData &vkInitData,
                                    vk::Extent3D extent,
                                    vk::Format format, vk::ImageUsageFlags usage,
                                    vk::ImageAspectFlags aspectFlags,
                                    uint32_t mipLevels,
                                    vk::SampleCountFlagBits samples);    
    void cleanupVulkanImage(VulkanInitData &vkInitData, VulkanImage &imageData);  
    
    void recreateAllVulkanDepthImages(  VulkanInitData &vkInitData, 
                                        vk::CommandBuffer &commandBuffer, 
                                        vector<VulkanImage> &allDepthImages);
    void cleanupAllVulkanDepthImages(   VulkanInitData &vkInitData, 
                                        vector<VulkanImage> &allDepthImages); 

    vk::RenderingAttachmentInfoKHR createDepthAttachment(VulkanImage &depthImage);
}