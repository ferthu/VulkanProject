#include "ConstantBufferVulkan.h"
#include "VulkanConstruct.h"


#pragma region Single buffered

ConstantBufferVulkan::ConstantBufferVulkan(VulkanRenderer *renderHandle)
	: buffer(NULL), _renderHandle(renderHandle), location(location)
{
}

ConstantBufferVulkan::~ConstantBufferVulkan()
{
	vkDestroyBuffer(_renderHandle->getDevice(), buffer, nullptr);
}

void ConstantBufferVulkan::transferData(const void* data, size_t byteSize, VkBufferUsageFlags usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
{
	if (!buffer)
	{
		memSize = 2 * byteSize;	//Technically allocated size might be larger due to the memory requirements.
		buffer = createBuffer(_renderHandle->getDevice(), memSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage);
		poolOffset = _renderHandle->bindPhysicalMemory(buffer, MemoryPool::UNIFORM_BUFFER);

		// Set the descriptor info

		if (!customDescriptor)
		{
			// Cyclic double buffer, generate two descriptors
			VkWriteDescriptorSet writes[1];
			VkDescriptorBufferInfo descriptorInfo[1];
			descriptorInfo[0].buffer = buffer;
			descriptorInfo[0].offset = 0;
			descriptorInfo[0].range = byteSize;

			if (hasFlag(usage, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT))
				writeDescriptorStruct_BUFFER(writes[0], descriptor, 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, descriptorInfo);
			else
				writeDescriptorStruct_BUFFER(writes[0], descriptor, 0, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, descriptorInfo);
			vkUpdateDescriptorSets(_renderHandle->getDevice(), 1, writes, 0, nullptr);
		}
		// Set initial frame data
		_renderHandle->transferBufferInitial(buffer, data, byteSize, 0);
	}
	else if (memSize < byteSize)
		throw std::runtime_error("Constant buffer cannot fit the data.");
	else
		_renderHandle->transferBufferData(buffer, data, byteSize, 0);
}

void ConstantBufferVulkan::setData(const void * data, size_t byteSize, uint32_t setBindIndex, VkDescriptorSetLayout layout, VkBufferUsageFlags usage)
{
	location = setBindIndex;
	if (!buffer && !customDescriptor)
	{
		// Gen the descriptor associated with the buffer
		if(hasFlag(usage, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT))
			descriptor = _renderHandle->generateDescriptor(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &layout);
		else
			descriptor = _renderHandle->generateDescriptor(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &layout);
	}
	transferData(data, byteSize, usage);
}

void ConstantBufferVulkan::setData(const void * data, size_t byteSize, uint32_t setBindIndex, VkBufferUsageFlags usage)
{
	location = setBindIndex;
	if (!buffer && !customDescriptor)
	{
		// Gen the descriptor associated with the buffer
		if (hasFlag(usage, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT))
			descriptor = _renderHandle->generateDescriptor(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, setBindIndex);
		else
			descriptor = _renderHandle->generateDescriptor(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, setBindIndex);
	}
	transferData(data, byteSize, usage);
}

void ConstantBufferVulkan::bind(VkCommandBuffer cmdBuf, VkPipelineLayout layout, VkPipelineBindPoint bindPoint)
{
	vkCmdBindDescriptorSets(cmdBuf, bindPoint, layout, location, 1, &descriptor, 0, nullptr);
}

VkBuffer ConstantBufferVulkan::getBuffer()
{
	return buffer;
}

void ConstantBufferVulkan::setUseCustomDescriptor(bool useCustomDescriptor)
{
	customDescriptor = useCustomDescriptor;
}

#pragma endregion



#pragma region Double buffered


ConstantDoubleBufferVulkan::ConstantDoubleBufferVulkan(VulkanRenderer *renderHandle)
	: _renderHandle(renderHandle)
{
	buffer[0] = NULL;
	buffer[1] = NULL;
}

ConstantDoubleBufferVulkan::~ConstantDoubleBufferVulkan()
{
	vkDestroyBuffer(_renderHandle->getDevice(), buffer[0], nullptr);
	vkDestroyBuffer(_renderHandle->getDevice(), buffer[1], nullptr);
}


void ConstantDoubleBufferVulkan::setData(const void * data, size_t byteSize, uint32_t setBindIndex, VkDescriptorSetLayout layout, VkBufferUsageFlags usage)
{
	location = setBindIndex;
	if (!buffer[0])
	{
		// Two buffers
		memSize = 2*byteSize;	//Technically allocated size might be larger due to the memory requirements.
		bufSize = byteSize;
		buffer[0] = createBuffer(_renderHandle->getDevice(), bufSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
		buffer[1] = createBuffer(_renderHandle->getDevice(), bufSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
		poolOffset = _renderHandle->bindPhysicalMemory(buffer[0], MemoryPool::UNIFORM_BUFFER);
		_renderHandle->bindPhysicalMemory(buffer[1], MemoryPool::UNIFORM_BUFFER);

		// Get & set the descriptor associated with the buffer
		if (hasFlag(usage, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT))
		{
			descriptor[0] = _renderHandle->generateDescriptor(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &layout);
			descriptor[1] = _renderHandle->generateDescriptor(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &layout);
		}
		else {
			descriptor[0] = _renderHandle->generateDescriptor(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &layout);
			descriptor[1] = _renderHandle->generateDescriptor(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &layout);
		}
		// Cycled double buffers
		VkWriteDescriptorSet writes[2];
		VkDescriptorBufferInfo descriptorInfo[2];
		descriptorInfo[0].buffer = buffer[0];
		descriptorInfo[0].offset = 0;
		descriptorInfo[0].range = byteSize;
		writeDescriptorStruct_UNI_BUFFER(writes[0], descriptor[0], 0, 0, 1, descriptorInfo);
		descriptorInfo[1].buffer = buffer[1];
		descriptorInfo[1].offset = 0;
		descriptorInfo[1].range = byteSize;
		writeDescriptorStruct_UNI_BUFFER(writes[1], descriptor[1], 0, 0, 1, &descriptorInfo[1]);
		vkUpdateDescriptorSets(_renderHandle->getDevice(), 2, writes, 0, nullptr);

		// Set initial frame data
		_renderHandle->transferBufferInitial(buffer[_renderHandle->getFrameIndex()], data, byteSize, 0);
		_renderHandle->transferBufferInitial(buffer[_renderHandle->getTransferIndex()], data, byteSize, 0);
	}
	else
		transferData(data, byteSize, usage);
}

void ConstantDoubleBufferVulkan::transferData(const void* data, size_t byteSize, VkBufferUsageFlags usage)
{
	if (!buffer[0])
		throw std::runtime_error("Constant buffer not initialized.");
	else if (memSize < byteSize)
		throw std::runtime_error("Constant buffer cannot fit the data.");
	else
		_renderHandle->transferBufferData(buffer[_renderHandle->getTransferIndex()], data, byteSize, 0);
}

void ConstantDoubleBufferVulkan::bind(VkCommandBuffer cmdBuf, VkPipelineLayout layout, VkPipelineBindPoint bindPoint)
{
	vkCmdBindDescriptorSets(cmdBuf, bindPoint, layout, location, 1, &descriptor[_renderHandle->getFrameIndex()], 0, nullptr);
}

#pragma endregion