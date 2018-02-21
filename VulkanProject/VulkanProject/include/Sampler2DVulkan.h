#pragma once
#include "Sampler2DVulkan.h"
#include <vulkan/vulkan.h>

class VulkanRenderer;

class Sampler2DVulkan
{
public:
	Sampler2DVulkan(VulkanRenderer *renderer);
	~Sampler2DVulkan();
	void setMagFilter(VkFilter filter);
	void setMinFilter(VkFilter filter);
	void setWrap(VkSamplerAddressMode s, VkSamplerAddressMode t);

	VulkanRenderer * _renderHandle;
	VkSampler _samplerHandle;

	VkFilter magFilter, minFilter;
	VkSamplerAddressMode wrap_s, wrap_t;

private:
	void destroySampler();
	void reCreateSampler();
};

