#include "TechniqueVulkan.h"
#include "ShaderVulkan.h"
#include "VulkanRenderer.h"
#include <iostream>
#include "VulkanConstruct.h"


/* Generate a compute pipeline technique
*/
TechniqueVulkan::TechniqueVulkan(VulkanRenderer* renderer, ShaderVulkan* sHandle, VkPipelineLayout layout)
	: _sHandle(sHandle), _renderHandle(renderer)
{
	createComputePipeline(layout);
}

/* Generate a graphics pipeline technique
*/
TechniqueVulkan::TechniqueVulkan( VulkanRenderer* renderer, ShaderVulkan* sHandle, VkRenderPass renderPass, VkPipelineLayout layout, VkPipelineVertexInputStateCreateInfo &vertexInputState)
	: _sHandle(sHandle), _renderHandle(renderer), _passHandle(renderPass)
{
	createGraphicsPipeline(layout, vertexInputState, 0);
}

TechniqueVulkan::TechniqueVulkan(VulkanRenderer* renderer, ShaderVulkan* sHandle, VkRenderPass renderPass, VkPipelineLayout layout, VkPipelineVertexInputStateCreateInfo &vertexInputState, uint32_t subpassIndex)
	: _sHandle(sHandle), _renderHandle(renderer), _passHandle(renderPass)
{
	createGraphicsPipeline(layout, vertexInputState, subpassIndex);
}

TechniqueVulkan::~TechniqueVulkan()
{
	vkDestroyPipeline(_renderHandle->getDevice(), pipeline, nullptr);
}

void TechniqueVulkan::bind(VkCommandBuffer cmdBuf, VkPipelineBindPoint bindPoint)
{
	vkCmdBindPipeline(cmdBuf, bindPoint, pipeline);
}

void TechniqueVulkan::createGraphicsPipeline(VkPipelineLayout layout, VkPipelineVertexInputStateCreateInfo &vertexInputState, uint32_t subpassIndex)
{
	assert(_sHandle);
	VkPipelineShaderStageCreateInfo stages[2];
	stages[0] = defineShaderStage(VK_SHADER_STAGE_VERTEX_BIT, _sHandle->vertexShader);
	stages[1] = defineShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, _sHandle->fragmentShader);
	
	//
	VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo =
		defineInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

	// Viewport
	VkViewport viewport = defineViewport((float)_renderHandle->getWidth(), (float)_renderHandle->getHeight());
	VkRect2D scissor = defineScissorRect(viewport);
	VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo =
		defineViewportState(&viewport, &scissor);

	// Rasterization state
	int rasterFlag = DEPTH_CLAMP_BIT;
	//if (rState->getWireframe())
	//	rasterFlag |= WIREFRAME_BIT;
	VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo =
		defineRasterizationState(rasterFlag, /*VK_CULL_MODE_BACK_BIT*/ VK_CULL_MODE_NONE);

	// Multisampling
	VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo =
		defineMultiSampling_OFF();

	// Blend states
	VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState = {};
	pipelineColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	pipelineColorBlendAttachmentState.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo =
		defineBlendState(&pipelineColorBlendAttachmentState, _sHandle->hasFragmentShader() ? 1 : 0);

	VkPipelineDepthStencilStateCreateInfo depthStencil =
		defineDepthState();

	const unsigned DYNAMIC_STATE_COUNT = 2;
	VkDynamicState viewportDynamicState[DYNAMIC_STATE_COUNT] = { VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT, VkDynamicState::VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.pNext = nullptr;
	dynamicStateCreateInfo.flags = 0;
	dynamicStateCreateInfo.dynamicStateCount = DYNAMIC_STATE_COUNT;
	dynamicStateCreateInfo.pDynamicStates = viewportDynamicState;

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = nullptr;
	pipelineInfo.flags = 0;
	pipelineInfo.stageCount = _sHandle->hasFragmentShader() ? 2 : 1;
	pipelineInfo.pStages = stages;
	pipelineInfo.pVertexInputState = &vertexInputState;
	pipelineInfo.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo;
	pipelineInfo.pTessellationState = nullptr;
	pipelineInfo.pViewportState = &pipelineViewportStateCreateInfo;
	pipelineInfo.pRasterizationState = &pipelineRasterizationStateCreateInfo;
	pipelineInfo.pMultisampleState = &pipelineMultisampleStateCreateInfo;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &pipelineColorBlendStateCreateInfo;
	pipelineInfo.pDynamicState = &dynamicStateCreateInfo;
	pipelineInfo.layout = layout;
	pipelineInfo.renderPass = _passHandle;
	pipelineInfo.subpass = subpassIndex;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = 0;

	VkResult err = vkCreateGraphicsPipelines(_renderHandle->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);

	if (err != VK_SUCCESS)
		throw std::runtime_error("Failed to create graphics pipeline.");
}

void TechniqueVulkan::createComputePipeline(VkPipelineLayout layout)
{
	assert(_sHandle);
	VkComputePipelineCreateInfo info;
	info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	info.pNext = NULL;
	info.flags = 0;
	info.stage = defineShaderStage(VK_SHADER_STAGE_COMPUTE_BIT, _sHandle->compShader);
	info.layout = layout;
	info.basePipelineHandle = NULL;
	info.basePipelineIndex = 0;

	VkResult err = vkCreateComputePipelines(_renderHandle->getDevice(), VK_NULL_HANDLE, 1, &info, NULL, &pipeline);
	if (err != VK_SUCCESS)
		throw std::runtime_error("Failed to create compute pipeline.");
}

