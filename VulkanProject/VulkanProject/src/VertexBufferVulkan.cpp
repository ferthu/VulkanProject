#include "VertexBufferVulkan.h"
#include <vulkan/vulkan.h>
#include <stdexcept>
#define VULKAN_DEVICE_IMPLEMENTATION
#include "../VulkanConstruct.h"
#include "VulkanRenderer.h"

VertexBufferVulkan::VertexBufferVulkan(VulkanRenderer *renderer, size_t size, VertexBuffer::DATA_USAGE usage)
	: _renderHandle(renderer), _bufferHandle(NULL), memSize(size)
{
	// Create buffer and allocater physical memory for it
	_bufferHandle = createBuffer(_renderHandle->getDevice(), size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	renderer->bindPhysicalMemory(_bufferHandle, MemoryPool::VERTEX_BUFFER);
}

VertexBufferVulkan::~VertexBufferVulkan()
{
	//Clean-up
	vkDestroyBuffer(_renderHandle->getDevice(), _bufferHandle, nullptr);
}

void VertexBufferVulkan::setData(const void * data, size_t size, size_t offset)
{
	_renderHandle->transferBufferInitial(_bufferHandle, data, size, offset);
}

void VertexBufferVulkan::bind(size_t offset, size_t size, unsigned int location)
{
	VkDeviceSize offsets[] = { offset };
	vkCmdBindVertexBuffers(_renderHandle->getFrameCmdBuf(), location, 1, &_bufferHandle, offsets);
}

void VertexBufferVulkan::unbind()
{
}

size_t VertexBufferVulkan::getSize()
{
	return memSize;
}
