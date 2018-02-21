#include "TechniqueVulkan.h"
#include "Vulkan\RenderStateVulkan.h"
#include "IA.h"
#include "Vulkan\MaterialVulkan.h"
#include <iostream>
#include "VulkanConstruct.h"


TechniqueVulkan::TechniqueVulkan(Material* m, RenderState* r, VulkanRenderer* renderer, VkRenderPass renderPass)
	: Technique(m, r), _renderHandle(renderer), _passHandle(renderPass)
{
	createPipeline();
}

TechniqueVulkan::~TechniqueVulkan()
{
	vkDestroyPipeline(_renderHandle->getDevice(), pipeline, nullptr);
}

void TechniqueVulkan::enable(Renderer* renderer)
{
	/* The render pipeline (includes RenderState) is set from the Vulkan Renderer.
	*/
	material->enable();
}

void TechniqueVulkan::createPipeline()
{
	MaterialVulkan *mat = dynamic_cast<MaterialVulkan*>(material);
	RenderStateVulkan *rState = dynamic_cast<RenderStateVulkan*>(renderState);
	assert(mat != NULL);
	VkPipelineShaderStageCreateInfo stages[2];
	stages[0] = defineShaderStage(VK_SHADER_STAGE_VERTEX_BIT,  mat->vertexShader);
	stages[1] = defineShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, mat->fragmentShader);

	// Vertex buffer bindings (static description...)
	const uint32_t NUM_BUFFER = 3;
	const uint32_t NUM_ATTRI = 3;
	VkVertexInputBindingDescription vertexBufferBindings[NUM_BUFFER] = 
	{
		defineVertexBinding(POSITION, 16),
		defineVertexBinding(NORMAL, 16),
		defineVertexBinding(TEXTCOORD, 8)
	};
	VkVertexInputAttributeDescription vertexAttributes[NUM_ATTRI] =
	{
		defineVertexAttribute(POSITION, POSITION, VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT, 0),
		defineVertexAttribute(NORMAL, NORMAL, VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT, 0),
		defineVertexAttribute(TEXTCOORD, TEXTCOORD, VkFormat::VK_FORMAT_R32G32_SFLOAT, 0)
	};
	VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = 
		defineVertexBufferBindings(vertexBufferBindings, NUM_BUFFER, vertexAttributes, NUM_ATTRI);

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
	if (rState->getWireframe())
		rasterFlag |= WIREFRAME_BIT;
	VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo =
		defineRasterizationState(rasterFlag, VK_CULL_MODE_NONE);

	// Multisampling
	VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo =
		defineMultiSampling_OFF();

	// Blend states
	VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState = {};
	pipelineColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	pipelineColorBlendAttachmentState.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo =
		defineBlendState(&pipelineColorBlendAttachmentState, 1);
	

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.pNext = nullptr;
	pipelineCreateInfo.flags = 0;
	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.pStages = stages;
	pipelineCreateInfo.pVertexInputState = &pipelineVertexInputStateCreateInfo;
	pipelineCreateInfo.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo;
	pipelineCreateInfo.pTessellationState = nullptr;
	pipelineCreateInfo.pViewportState = &pipelineViewportStateCreateInfo;
	pipelineCreateInfo.pRasterizationState = &pipelineRasterizationStateCreateInfo;
	pipelineCreateInfo.pMultisampleState = &pipelineMultisampleStateCreateInfo;
	pipelineCreateInfo.pDepthStencilState = nullptr;
	pipelineCreateInfo.pColorBlendState = &pipelineColorBlendStateCreateInfo;
	pipelineCreateInfo.pDynamicState = rState->getDynamicState();
	pipelineCreateInfo.layout = mat->pipelineLayout;
	pipelineCreateInfo.renderPass = _passHandle;
	pipelineCreateInfo.subpass = 0;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineCreateInfo.basePipelineIndex = 0;

	VkResult err = vkCreateGraphicsPipelines(_renderHandle->getDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline);

	if (err != VK_SUCCESS)
		throw std::runtime_error("Failed to create pipeline.");
}


