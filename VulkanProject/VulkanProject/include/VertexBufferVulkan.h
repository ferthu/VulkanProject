#pragma once
#include<vulkan/vulkan.h>


class VulkanRenderer;

class VertexBufferVulkan
{
public:
	enum DATA_USAGE { STATIC = 0, DYNAMIC = 1, DONTCARE = 2 };

	VertexBufferVulkan(VulkanRenderer *renderHandle, size_t size, DATA_USAGE usage);
	~VertexBufferVulkan();

	void setData(const void* data, size_t size, size_t offset);
	void bind(size_t offset, size_t size, unsigned int location);
	void unbind();
	size_t getSize();
	VkBuffer _bufferHandle;
private:

	VulkanRenderer* _renderHandle;
	size_t memSize;
};

