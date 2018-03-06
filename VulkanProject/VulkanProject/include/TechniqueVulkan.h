#pragma once
#pragma once
#include "vulkan\vulkan.h"
#include <map>

class ShaderVulkan;
class VulkanRenderer;

class TechniqueVulkan
{
public:
	/* Generate a compute pipeline technique
	*/
	TechniqueVulkan(VulkanRenderer* renderer, ShaderVulkan* sHandle, VkPipelineLayout layout);
	/* Generate a graphics pipeline technique
	*/
	TechniqueVulkan(VulkanRenderer* renderer, ShaderVulkan* sHandle, VkRenderPass renderPass, VkPipelineLayout layout, VkPipelineVertexInputStateCreateInfo &vertexInputState);
	virtual ~TechniqueVulkan();
	virtual void bind(VkCommandBuffer cmdBuf, VkPipelineBindPoint bindPoint);

	VkPipeline pipeline;

private:
	void createGraphicsPipeline(VkPipelineLayout layout, VkPipelineVertexInputStateCreateInfo &vertexInputState);
	void createComputePipeline(VkPipelineLayout layout);

	VulkanRenderer *_renderHandle;
	ShaderVulkan *_sHandle;
	VkRenderPass _passHandle;
	
	
};

