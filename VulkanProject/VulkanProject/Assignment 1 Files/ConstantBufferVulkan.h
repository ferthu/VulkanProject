#pragma once
#include <GL/glew.h>
#include "../ConstantBuffer.h"
#include "VulkanRenderer.h"
#include "vulkan\vulkan.h"

class MaterialVulkan;

class ConstantBufferVulkan : public ConstantBuffer
{
public:
	ConstantBufferVulkan(std::string NAME, unsigned int location);
	virtual ~ConstantBufferVulkan();
	void setData(const void* data, size_t size, Material* m, unsigned int location);
	void bind(Material*);
	void init(VulkanRenderer* renderer);

	VkDescriptorBufferInfo* getDescriptorBufferInfo();
	
private:

	VkDescriptorSet descriptor;

	VulkanRenderer* _renderHandle;

	std::string name;
	uint32_t location;
	size_t poolOffset, memSize;

	VkBuffer buffer;
	VkDescriptorBufferInfo descriptorBufferInfo;
};

