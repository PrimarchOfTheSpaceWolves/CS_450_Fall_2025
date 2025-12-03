#include "student/VKImage.hpp"

namespace student {
    VulkanImageTransition createVulkanImageTransition(vk::Image &image, VK_IMAGE_TRANSITION_TYPE type) {        
        VulkanImageTransition transitionData {};

        vk::ImageLayout oldLayout {};
        vk::ImageLayout newLayout {};
        vk::AccessFlags srcMask {};
        vk::AccessFlags dstMask {};
        vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eColor;

        switch(type) {
            case UNDEF_TO_COLOR:
            {
                oldLayout = vk::ImageLayout::eUndefined;
                newLayout = vk::ImageLayout::eColorAttachmentOptimal;
                dstMask = vk::AccessFlagBits::eColorAttachmentWrite;

                transitionData.srcFlags = vk::PipelineStageFlagBits::eTopOfPipe;
                transitionData.dstFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
                break;
            }            
            case COLOR_TO_PRESENT:
            {
                oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
                newLayout = vk::ImageLayout::ePresentSrcKHR;
                srcMask = vk::AccessFlagBits::eColorAttachmentWrite;   
                
                transitionData.srcFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
                transitionData.dstFlags = vk::PipelineStageFlagBits::eBottomOfPipe;
                break;
            }
            case UNDEF_TO_DEPTH:
            {
                oldLayout = vk::ImageLayout::eUndefined;
                newLayout = vk::ImageLayout::eDepthAttachmentOptimal;
                dstMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite
                             | vk::AccessFlagBits::eDepthStencilAttachmentRead;

                transitionData.srcFlags = vk::PipelineStageFlagBits::eTopOfPipe;
                transitionData.dstFlags = vk::PipelineStageFlagBits::eEarlyFragmentTests
                                         | vk::PipelineStageFlagBits::eLateFragmentTests;
                
                aspectFlags = vk::ImageAspectFlagBits::eDepth;
                break;
            }            
            default:
            {
                throw invalid_argument("Unsupported layout transition!");
                break;
            }
        }

        // Create the actual memory barrier
        vk::ImageMemoryBarrier barrier {};
        barrier.setOldLayout(oldLayout);
        barrier.setNewLayout(newLayout);
        barrier.setSrcAccessMask(srcMask);
        barrier.setDstAccessMask(dstMask);
        barrier.setImage(image);
        barrier.setSubresourceRange(
            vk::ImageSubresourceRange(aspectFlags, 0, 1, 0, 1));

        transitionData.barrier = barrier;

        // Return data
        return transitionData;
    }

    void performVulkanImageTransition(vk::CommandBuffer &commandBuffer, VulkanImageTransition &transitionData) {
        commandBuffer.pipelineBarrier(
            transitionData.srcFlags,
            transitionData.dstFlags,
            {}, nullptr, nullptr,
            transitionData.barrier);
    }

    VulkanImage createVulkanImage(  VulkanInitData &vkInitData,
                                    vk::Extent3D extent,
                                    vk::Format format, vk::ImageUsageFlags usage,
                                    vk::ImageAspectFlags aspectFlags,
                                    uint32_t mipLevels,
                                    vk::SampleCountFlagBits samples) {

        // Create struct
        VulkanImage imageData{};
        imageData.extent = extent;
        imageData.format = format;
        imageData.mipLevels = mipLevels;
        
        // Set up creation info
        vk::ImageCreateInfo imgInfo {};
        imgInfo.imageType = vk::ImageType::e2D;
        imgInfo.extent = extent;
        imgInfo.mipLevels = mipLevels;        
        imgInfo.samples = samples;
        imgInfo.format = format;
        imgInfo.usage = usage;
        imgInfo.initialLayout = vk::ImageLayout::eUndefined;// Start undefined 
                                                            // (will have to transition later)
        imgInfo.arrayLayers = 1;                            // Not an array
        imgInfo.tiling = vk::ImageTiling::eOptimal;         // Layout memory efficiently 
                                                            // (can't read texels easily ourselves)
        imgInfo.sharingMode = vk::SharingMode::eExclusive;  // Only used by one queue family

        // Want data on device
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        // Actually allocate data
        VkImageCreateInfo vkImgInfo = static_cast<VkImageCreateInfo>(imgInfo);
        VkImage image;
        VmaAllocation alloc;
        vmaCreateImage(vkInitData.allocator, &vkImgInfo, &allocInfo, 
                        &image, &alloc, nullptr);

        // Save image and allocation
        imageData.image = vk::Image { image };
        imageData.allocation = alloc;

        // Also create image view while we're here
        vk::ImageViewCreateInfo viewInfo {};
        viewInfo.image = imageData.image;
        viewInfo.format = format;
        viewInfo.viewType = vk::ImageViewType::e2D;
        viewInfo.subresourceRange = { aspectFlags, 0, 1, 0, 1 }; 
        // Aspect that are visible (also mipmap level and array ranges)
   
        imageData.view = vkInitData.device.createImageView(viewInfo);

        // Return data
        return imageData;
    }

    void cleanupVulkanImage(VulkanInitData &vkInitData, VulkanImage &imageData) {
        vkInitData.device.destroyImageView(imageData.view);
        vmaDestroyImage(vkInitData.allocator, imageData.image, imageData.allocation);
    }

    void recreateAllVulkanDepthImages(  VulkanInitData &vkInitData, 
                                        vk::CommandBuffer &commandBuffer, 
                                        vector<VulkanImage> &allDepthImages) {
        if(allDepthImages.size() > 0) {            
            cleanupAllVulkanDepthImages(vkInitData, allDepthImages);
        }

        for(int i = 0; i < vkInitData.swapchain.images.size(); i++) {
            VulkanImage depthImage =  createVulkanImage(   
                                    vkInitData, 
                                    vk::Extent3D { 
                                        vkInitData.swapchain.extent.width, 
                                        vkInitData.swapchain.extent.height, 
                                        1 },
                                    vk::Format::eD32Sfloat,
                                    vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                    vk::ImageAspectFlagBits::eDepth,
                                    1, vk::SampleCountFlagBits::e1);          
            allDepthImages.push_back(depthImage);
            VulkanImageTransition depthBarrier = createVulkanImageTransition(
                depthImage.image, VK_IMAGE_TRANSITION_TYPE::UNDEF_TO_DEPTH);
            performVulkanImageTransition(commandBuffer, depthBarrier);
        }
    } 

    void cleanupAllVulkanDepthImages(VulkanInitData &vkInitData, vector<VulkanImage> &allDepthImages) {
        for(int i = 0; i < allDepthImages.size(); i++) {
            cleanupVulkanImage(vkInitData, allDepthImages.at(i));
        }
        allDepthImages.clear();
    }

    vk::RenderingAttachmentInfoKHR createDepthAttachment(VulkanImage &depthImage) {
        vk::RenderingAttachmentInfoKHR depthAtt{};
        depthAtt.setImageView(depthImage.view)
                .setImageLayout(vk::ImageLayout::eDepthAttachmentOptimal)
                .setLoadOp(vk::AttachmentLoadOp::eClear)
                .setStoreOp(vk::AttachmentStoreOp::eStore)
                .setClearValue(vk::ClearDepthStencilValue {1.0f, 0});
        return depthAtt;
    }
}