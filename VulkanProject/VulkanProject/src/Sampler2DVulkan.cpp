#include "Sampler2DVulkan.h"
#include "../VulkanConstruct.h"
#include "VulkanRenderer.h"

Sampler2DVulkan::Sampler2DVulkan(VulkanRenderer *renderer)
	: _renderHandle(renderer), _samplerHandle(NULL),
	magFilter(VK_FILTER_LINEAR), minFilter(VK_FILTER_LINEAR),
	wrap_s(VK_SAMPLER_ADDRESS_MODE_REPEAT), wrap_t(VK_SAMPLER_ADDRESS_MODE_REPEAT)
{
}

Sampler2DVulkan::~Sampler2DVulkan()
{
	destroySampler();
}

void Sampler2DVulkan::setMagFilter(VkFilter filter)
{
	magFilter = filter;
	reCreateSampler();
}

void Sampler2DVulkan::setMinFilter(VkFilter filter)
{
	minFilter = filter;
	reCreateSampler();
}


void Sampler2DVulkan::setWrap(VkSamplerAddressMode s, VkSamplerAddressMode t)
{
	wrap_s = s;
	wrap_t = t;
	reCreateSampler();
}


void Sampler2DVulkan::destroySampler()
{
	if(_samplerHandle)
		vkDestroySampler(_renderHandle->getDevice(), _samplerHandle, nullptr);
}

void Sampler2DVulkan::reCreateSampler()
{
	destroySampler();
	_samplerHandle = createSampler(_renderHandle->getDevice(), magFilter, minFilter, wrap_s, wrap_t);
}
