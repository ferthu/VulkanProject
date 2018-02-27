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
	TechniqueVulkan(ShaderVulkan* sHandle, VulkanRenderer* renderer);
	/* Generate a graphics pipeline technique
	*/
	TechniqueVulkan(ShaderVulkan* sHandle, VulkanRenderer* renderer, VkRenderPass renderPass, VkPipelineVertexInputStateCreateInfo &vertexInputState);
	virtual ~TechniqueVulkan();
	virtual void bind(VkCommandBuffer cmdBuf, VkPipelineBindPoint bindPoint);

	VkPipeline pipeline;

private:
	void createGraphicsPipeline(VkPipelineVertexInputStateCreateInfo &vertexInputState);
	void createComputePipeline();

	VulkanRenderer *_renderHandle;
	ShaderVulkan *_sHandle;
	VkRenderPass _passHandle;
	
	
};

