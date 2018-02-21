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


VkSamplerAddressMode translate(WRAPPING m)
{
	if (m == WRAPPING::REPEAT)
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	else //m == WRAPPING::CLAMP
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
}
VkFilter translate(FILTER m)
{
	if (m == FILTER::LINEAR)
		return VK_FILTER_LINEAR;
	else //m == FILTER::POINT_SAMPLER
		return VK_FILTER_NEAREST;
}

void Sampler2DVulkan::setMagFilter(FILTER filter)
{
	magFilter = translate(filter);
	reCreateSampler();
}

void Sampler2DVulkan::setMinFilter(FILTER filter)
{
	minFilter = translate(filter);
	reCreateSampler();
}


void Sampler2DVulkan::setWrap(WRAPPING s, WRAPPING t)
{
	wrap_s = translate(s);
	wrap_t = translate(t);
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
