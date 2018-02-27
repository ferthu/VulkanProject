#pragma once

#include <vulkan/vulkan.h>
#include <string>

class Sampler2DVulkan;
class VulkanRenderer;

const int MAX_TEX_BINDINGS = 6;

class Texture2DVulkan 
{
private:
	void destroyImg();
	VkDescriptorSet slotBindings[MAX_TEX_BINDINGS];

public:
	Texture2DVulkan(VulkanRenderer *renderer, Sampler2DVulkan *sampler);
	~Texture2DVulkan();

	int loadFromFile(std::string filename);
	void bind(VkCommandBuffer cmdBuf, unsigned int slot);

	VulkanRenderer *_renderHandle;
	Sampler2DVulkan *_samplerHandle;
	VkImage _imageHandle;
	VkDescriptorImageInfo imageInfo;
};

