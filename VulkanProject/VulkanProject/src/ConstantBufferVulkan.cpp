#include "ConstantBufferVulkan.h"
#include "VulkanConstruct.h"
#include "MaterialVulkan.h"
ConstantBufferVulkan::ConstantBufferVulkan(std::string NAME, unsigned int location)
	: buffer()
{
	buffer[0] = NULL;
	buffer[1] = NULL;
	name = NAME;
	this->location = location;
}

ConstantBufferVulkan::~ConstantBufferVulkan()
{
	vkDestroyBuffer(_renderHandle->getDevice(), buffer[0], nullptr);
	vkDestroyBuffer(_renderHandle->getDevice(), buffer[1], nullptr);
}


void ConstantBufferVulkan::setData(const void * data, size_t size, VulkanMaterial * m, unsigned int location)
{
	if (!buffer[0])
	{
		memSize = 2*size;	//Technically allocated size might be larger due to the memory requirements.
		buffer[0] = createBuffer(_renderHandle->getDevice(), memSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
		buffer[1] = createBuffer(_renderHandle->getDevice(), memSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
		poolOffset = _renderHandle->bindPhysicalMemory(buffer[0], MemoryPool::UNIFORM_BUFFER);
		_renderHandle->bindPhysicalMemory(buffer[1], MemoryPool::UNIFORM_BUFFER);

		// Set the descriptor info

		// Get & set the descriptor associated with the buffer
		descriptor[0] = _renderHandle->generateDescriptor(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, location);
		descriptor[1] = _renderHandle->generateDescriptor(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, location);

		// Cyclic double buffer, generate two descriptors
		VkWriteDescriptorSet writes[2];
		VkDescriptorBufferInfo descriptorInfo[2];
		descriptorInfo[0].buffer = buffer[0];
		descriptorInfo[0].offset = 0;
		descriptorInfo[0].range = size;
		writeDescriptorStruct_UNI_BUFFER(writes[0], descriptor[0], 0, 0, 1, descriptorInfo);
		descriptorInfo[1].buffer = buffer[1];
		descriptorInfo[1].offset = 0;
		descriptorInfo[1].range = size;
		writeDescriptorStruct_UNI_BUFFER(writes[1], descriptor[1], 0, 0, 1, &descriptorInfo[1]);
		vkUpdateDescriptorSets(_renderHandle->getDevice(), 2, writes, 0, nullptr);

		// Set initial frame data
		_renderHandle->transferBufferInitial(buffer[_renderHandle->getFrameIndex()], data, size, 0);
		_renderHandle->transferBufferInitial(buffer[_renderHandle->getTransferIndex()], data, size, 0);
	}
	else if(memSize / 2 < size)
		throw std::runtime_error("Constant buffer cannot fit the data.");
	else
		_renderHandle->transferBufferData(buffer[_renderHandle->getTransferIndex()], data, size, 0);
}

void ConstantBufferVulkan::bind(VulkanMaterial *m)
{
	vkCmdBindDescriptorSets(_renderHandle->getFrameCmdBuf(), VK_PIPELINE_BIND_POINT_GRAPHICS, _renderHandle->getPipelineLayout(), location, 1, &descriptor[_renderHandle->getFrameIndex()], 0, nullptr);
}

void ConstantBufferVulkan::init(VulkanRenderer* renderer)
{
	this->_renderHandle = renderer;
}
