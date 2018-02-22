#include "TechniqueVulkan.h"
#include "ShaderVulkan.h"
#include "VulkanRenderer.h"
#include <iostream>
#include "VulkanConstruct.h"


TechniqueVulkan::TechniqueVulkan(ShaderVulkan* sHandle, VulkanRenderer* renderer, VkRenderPass renderPass, VkPipelineVertexInputStateCreateInfo &vertexInputState)
	: _sHandle(sHandle), _renderHandle(renderer), _passHandle(renderPass)
{
	createPipeline(vertexInputState);
}

TechniqueVulkan::~TechniqueVulkan()
{
	vkDestroyPipeline(_renderHandle->getDevice(), pipeline, nullptr);
}

void TechniqueVulkan::enable()
{
	/* The render pipeline (includes RenderState) is set from the Vulkan Renderer.
	*/
}

void TechniqueVulkan::createPipeline(VkPipelineVertexInputStateCreateInfo &vertexInputState)
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
	int rasterFlag = 0;
	//if (rState->getWireframe())
	//	rasterFlag |= WIREFRAME_BIT;
	VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo =
		defineRasterizationState(rasterFlag, VK_CULL_MODE_BACK_BIT);

	// Multisampling
	VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo =
		defineMultiSampling_OFF();

	// Blend states
	VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState = {};
	pipelineColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	pipelineColorBlendAttachmentState.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo =
		defineBlendState(&pipelineColorBlendAttachmentState, 1);

	VkPipelineDepthStencilStateCreateInfo depthStencil =
		defineDepthState();

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = nullptr;
	pipelineInfo.flags = 0;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = stages;
	pipelineInfo.pVertexInputState = &vertexInputState;
	pipelineInfo.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo;
	pipelineInfo.pTessellationState = nullptr;
	pipelineInfo.pViewportState = &pipelineViewportStateCreateInfo;
	pipelineInfo.pRasterizationState = &pipelineRasterizationStateCreateInfo;
	pipelineInfo.pMultisampleState = &pipelineMultisampleStateCreateInfo;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &pipelineColorBlendStateCreateInfo;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.layout = _renderHandle->getPipelineLayout();
	pipelineInfo.renderPass = _passHandle;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = 0;

	VkResult err = vkCreateGraphicsPipelines(_renderHandle->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);

	if (err != VK_SUCCESS)
		throw std::runtime_error("Failed to create pipeline.");
}


