#include "Texture2DVulkan.h"
#include "Sampler2DVulkan.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "VulkanConstruct.h"
#include "VulkanRenderer.h"
#include<assert.h>

Texture2DVulkan::Texture2DVulkan(VulkanRenderer *renderer, Sampler2DVulkan *sampler)
	: _renderHandle(renderer), _samplerHandle(sampler), _imageHandle(nullptr), imageInfo({NULL, NULL, VK_IMAGE_LAYOUT_UNDEFINED })
{
	for (int i = 0; i < MAX_TEX_BINDINGS; i++)
		slotBindings[i] = NULL;
}

Texture2DVulkan::~Texture2DVulkan()
{
	destroyImg();
}

void Texture2DVulkan::destroyImg()
{
	if (_imageHandle)
	{
		vkDestroyImageView(_renderHandle->getDevice(), imageInfo.imageView, nullptr);
		vkDestroyImage(_renderHandle->getDevice(), _imageHandle, nullptr);
	}
}

int Texture2DVulkan::loadFromFile(std::string filename)
{
	int w, h, bpp;
	unsigned char* rgb = stbi_load(filename.c_str(), &w, &h, &bpp, STBI_rgb_alpha);
	if (rgb == nullptr)
	{
		fprintf(stderr, "Error loading texture file: %s\n", filename.c_str());
		return -1;
	}

	// Destroy image if it exists
	destroyImg();

	VkFormat format;
	uint32_t bytes; 
	if (bpp == 3)
	{
		//Warning! Might not be supported
		format = VK_FORMAT_R8G8B8_UNORM;
		bytes = 3;
	}
	else if (bpp == 4)
	{
		// Standard support format
		format = VK_FORMAT_R8G8B8A8_UNORM;
		bytes = 4;
	}
	else
		throw std::runtime_error("Image format not supported...");
	_imageHandle = createTexture2D(_renderHandle->getDevice(), w, h, format);
	_renderHandle->bindPhysicalMemory(_imageHandle, MemoryPool::IMAGE_RGBA8_BUFFER);
	// Create and transfer image
	_renderHandle->transitionImageLayout(_imageHandle, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	_renderHandle->transferImageData(_imageHandle, rgb, glm::uvec3(w, h, 1), bytes);
	stbi_image_free(rgb);
	_renderHandle->transitionImageLayout(_imageHandle, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// Create image view
	imageInfo.imageView = createImageView(_renderHandle->getDevice(), _imageHandle, format);
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	return 0;
}

void Texture2DVulkan::createShadowMap(uint32_t height, uint32_t width, VkFormat shadowMapFormat)
{
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.pNext = nullptr;
	imageCreateInfo.flags = 0;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.extent.width = width;
	imageCreateInfo.extent.height = height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.format = shadowMapFormat;
	imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
	if (vkCreateImage(_renderHandle->getDevice(), &imageCreateInfo, nullptr, &_imageHandle) != VK_SUCCESS)
		throw std::runtime_error("Failed to create shadow map");

	_renderHandle->bindPhysicalMemory(_imageHandle, MemoryPool::IMAGE_D16_BUFFER);

	// Create image view
	VkImageViewCreateInfo depthStencilView = {};
	depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	depthStencilView.pNext = nullptr;
	depthStencilView.flags = 0;
	depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthStencilView.format = shadowMapFormat;
	depthStencilView.subresourceRange = {};
	depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	depthStencilView.subresourceRange.baseMipLevel = 0;
	depthStencilView.subresourceRange.levelCount = 1;
	depthStencilView.subresourceRange.baseArrayLayer = 0;
	depthStencilView.subresourceRange.layerCount = 1;
	depthStencilView.image = _imageHandle;
	if (vkCreateImageView(_renderHandle->getDevice(), &depthStencilView, nullptr, &imageInfo.imageView) != VK_SUCCESS)
		throw std::runtime_error("Failed to create shadow map view");

	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}


void Texture2DVulkan::attachBindPoint(uint32_t attachmentIndex, VkDescriptorSetLayout layout)
{
	if (!slotBindings[attachmentIndex])
		// Descriptor
		slotBindings[attachmentIndex] = _renderHandle->generateDescriptor(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &layout);

	imageInfo.sampler = _samplerHandle->_sampler;
	VkWriteDescriptorSet writes[1];
	writeDescriptorStruct_IMG_COMBINED(writes[0], slotBindings[attachmentIndex], 0, 0, 1, &imageInfo);
	vkUpdateDescriptorSets(_renderHandle->getDevice(), 1, writes, 0, nullptr);

}

void Texture2DVulkan::bind(VkCommandBuffer cmdBuf, uint32_t indexCombined, VkPipelineLayout layout, VkPipelineBindPoint pipeBinding)
{
	assert(slotBindings[indexCombined] != NULL);
	vkCmdBindDescriptorSets(cmdBuf, pipeBinding, layout, indexCombined, 1, &slotBindings[indexCombined], 0, nullptr);
}
void Texture2DVulkan::bind(VkCommandBuffer cmdBuf, uint32_t attachmentIndex, uint32_t setIndex, VkPipelineLayout layout, VkPipelineBindPoint pipeBinding)
{
	assert(slotBindings[attachmentIndex] != NULL);
	vkCmdBindDescriptorSets(cmdBuf, pipeBinding, layout, setIndex, 1, &slotBindings[attachmentIndex], 0, nullptr);
}
