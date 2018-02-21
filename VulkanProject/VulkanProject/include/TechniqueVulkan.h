#pragma once
#pragma once
#include "vulkan\vulkan.h"
#include <map>

class ShaderVulkan;
class VulkanRenderer;

class TechniqueVulkan
{
public:
	TechniqueVulkan(ShaderVulkan* sHandle, VulkanRenderer* renderer, VkRenderPass renderPass);
	virtual ~TechniqueVulkan();
	virtual void enable();

	VkPipeline pipeline;

private:
	void createPipeline();

	VulkanRenderer *_renderHandle;
	ShaderVulkan *_sHandle;
	VkRenderPass _passHandle;
	
	
};

