#pragma once
#include "VulkanRenderer.h"
#include "vulkan\vulkan.h"

class ShaderVulkan;

/* Single buffered Uniform buffer
*/
class ConstantBufferVulkan
{
public:
	ConstantBufferVulkan(VulkanRenderer *renderHandle, std::string NAME, unsigned int location);
	virtual ~ConstantBufferVulkan();
	void setData(const void* data, size_t size, unsigned int location);
	void bind();

private:
	// Double buffered:

	VkDescriptorSet descriptor;
	VkBuffer buffer;

	VulkanRenderer* _renderHandle;

	std::string name;
	uint32_t location;
	size_t poolOffset, memSize;

};

/* Double buffered Uniform buffer
*/
class ConstantDoubleBufferVulkan
{
public:
	ConstantDoubleBufferVulkan(VulkanRenderer *renderHandle, std::string NAME, unsigned int location);
	virtual ~ConstantDoubleBufferVulkan();
	void setData(const void* data, size_t size, unsigned int location);
	void bind();

private:
	// Double buffered:

	VkDescriptorSet descriptor[2];
	VkBuffer buffer[2];

	VulkanRenderer* _renderHandle;

	std::string name;
	uint32_t location;
	size_t poolOffset, memSize;

};

