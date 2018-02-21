#pragma once
#include "../Sampler2D.h"
#include <vulkan/vulkan.h>

class VulkanRenderer;

class Sampler2DVulkan :
	public Sampler2D
{
public:
	Sampler2DVulkan(VulkanRenderer *renderer);
	~Sampler2DVulkan();
	void setMagFilter(FILTER filter);
	void setMinFilter(FILTER filter);
	void setWrap(WRAPPING s, WRAPPING t);

	VulkanRenderer * _renderHandle;
	VkSampler _samplerHandle;

	VkFilter magFilter, minFilter;
	VkSamplerAddressMode wrap_s, wrap_t;

private:
	void destroySampler();
	void reCreateSampler();
};

