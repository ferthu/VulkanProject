#pragma once
#include "VulkanRenderer.h"
#include "vulkan\vulkan.h"

class ShaderVulkan;

/* Single buffered Uniform buffer
*/
class ConstantBufferVulkan
{
public:
	ConstantBufferVulkan(VulkanRenderer *renderHandle);
	virtual ~ConstantBufferVulkan();
	void setData(const void* data, size_t byteSize, uint32_t setBindIndex, VkBufferUsageFlags usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	void setData(const void * data, size_t byteSize, uint32_t setBindIndex, VkDescriptorSetLayout layout, VkBufferUsageFlags usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	void bind(VkCommandBuffer cmdBuf, VkPipelineLayout layout, VkPipelineBindPoint bindPoint= VK_PIPELINE_BIND_POINT_GRAPHICS);
	VkBuffer getBuffer();
	void setUseCustomDescriptor(bool useCustomDescriptor);

private:
	// Single buffered:

	void transferData(const void* data, size_t byteSize, VkBufferUsageFlags usage);
	VkDescriptorSet descriptor;
	VkBuffer buffer;

	VulkanRenderer* _renderHandle;

	uint32_t location;
	size_t poolOffset, memSize;

	// True if user handles descriptors and binding on their own
	// Makes the constant buffer ignore its own descriptor handling
	bool customDescriptor = false;
};

/* Double buffered Uniform buffer
*/
class ConstantDoubleBufferVulkan
{
public:
	ConstantDoubleBufferVulkan(VulkanRenderer *renderHandle, std::string NAME, unsigned int location);
	virtual ~ConstantDoubleBufferVulkan();
	void setData(const void* data, size_t size, unsigned int location);
	void bind(VkCommandBuffer cmdBuf, VkPipelineLayout layout, VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS);

private:
	// Double buffered:

	VkDescriptorSet descriptor[2];
	VkBuffer buffer[2];

	VulkanRenderer* _renderHandle;

	std::string name;
	uint32_t location;
	size_t poolOffset, memSize;

};

