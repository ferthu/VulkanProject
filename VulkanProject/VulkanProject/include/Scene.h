#pragma once
#include "vulkan\vulkan.h"
#include <vector>

class VulkanRenderer;

/* Scene or Frame implementation
*/
class Scene
{
public:

	VulkanRenderer * _renderHandle;

	virtual void defineDescriptorLayout(VkDevice device, std::vector<VkDescriptorSetLayout> &layout) = 0;

	virtual void initialize(VulkanRenderer *handle) { _renderHandle = handle; };
	virtual void frame(VkCommandBuffer cmdBuf) = 0;
	virtual ~Scene() {};
};