#include "ConstantBufferVulkan.h"
#include "../VulkanConstruct.h"
#include "MaterialVulkan.h"
ConstantBufferVulkan::ConstantBufferVulkan(std::string NAME, unsigned int location)
	: buffer(nullptr)
{
	name = NAME;
	this->location = location;
}

ConstantBufferVulkan::~ConstantBufferVulkan()
{
	vkDestroyBuffer(_renderHandle->getDevice(), buffer, nullptr);
}


void ConstantBufferVulkan::setData(const void * data, size_t size, Material * m, unsigned int location)
{
	if (!buffer)
	{
		buffer = createBuffer(_renderHandle->getDevice(), size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
		memSize = size;	//Technically size might be larger due to the memory requirements.
		poolOffset = _renderHandle->bindPhysicalMemory(buffer, MemoryPool::UNIFORM_BUFFER);
		// Set the descriptor info
		descriptorBufferInfo.buffer = buffer;
		descriptorBufferInfo.offset = 0;
		descriptorBufferInfo.range = VK_WHOLE_SIZE;

		// Get & set the descriptor associated with the buffer
		descriptor = _renderHandle->generateDescriptor(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, ((MaterialVulkan*)m)->getLayoutBinding(location));
		VkWriteDescriptorSet writes[1];
		writeDescriptorStruct_UNI_BUFFER(writes[0], descriptor, 0, 0, 1, &descriptorBufferInfo);
		vkUpdateDescriptorSets(_renderHandle->getDevice(), 1, writes, 0, nullptr);
	}
	else if(memSize < size)
		throw std::runtime_error("Constant buffer cannot fit the data.");
	_renderHandle->transferBufferData(buffer, data, size, 0);
}

void ConstantBufferVulkan::bind(Material *m)
{
	assert(dynamic_cast<MaterialVulkan*>(m));
	vkCmdBindDescriptorSets(_renderHandle->getFrameCmdBuf(), VK_PIPELINE_BIND_POINT_GRAPHICS, ((MaterialVulkan*)m)->pipelineLayout, location, 1, &descriptor, 0, nullptr);
}

void ConstantBufferVulkan::init(VulkanRenderer* renderer)
{
	this->_renderHandle = renderer;
}

VkDescriptorBufferInfo* ConstantBufferVulkan::getDescriptorBufferInfo()
{
	return &descriptorBufferInfo;
}
