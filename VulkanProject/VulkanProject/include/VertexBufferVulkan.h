#pragma once
#include<vulkan/vulkan.h>


class VulkanRenderer;

class VertexBufferVulkan
{
public:
	enum DATA_USAGE { STATIC = 0, DYNAMIC = 1, DONTCARE = 2 };
	/* Binding used to bind vertex buffers.
	*/
	struct Binding {
		uint32_t sizeElement, numElements, offset;
		VertexBufferVulkan* buffer;
		void bind(VkCommandBuffer cmdBuf, uint32_t location);
	
		Binding();
		Binding(VertexBufferVulkan* buffer, uint32_t sizeElement, uint32_t numElements, uint32_t offset);
		size_t byteSize() { return sizeElement * numElements; }
	};


	VertexBufferVulkan(VulkanRenderer *renderHandle, size_t size, DATA_USAGE usage);
	~VertexBufferVulkan();

	void setData(const void* data, size_t size, size_t offset);
	void setData(const void* data, Binding& binding);
	void bind(VkCommandBuffer cmdBuf, size_t offset, size_t size, unsigned int location);
	void unbind();
	size_t getSize();
	VkBuffer _bufferHandle;
private:

	VulkanRenderer* _renderHandle;
	size_t memSize;
};

