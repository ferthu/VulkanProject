#include "Scenes/ShadowScene.h"
#include "VulkanRenderer.h"
#include "Stuff/RandomGenerator.h"

ShadowScene::ShadowScene()
{

}

ShadowScene::~ShadowScene()
{
	delete positionBuffer;
	delete normalBuffer;

	delete shadowMapSampler;

	delete depthPassTechnique;
	delete depthPassShaders;

	delete renderPassTechnique;
	delete renderPassShaders;
}

void ShadowScene::initialize(VulkanRenderer* handle)
{
	Scene::initialize(handle);

	// Create triangles
	const uint32_t TRIANGLE_COUNT = 100;
	glm::vec4 vertexPositions[TRIANGLE_COUNT * 3];
	glm::vec3 vertexNormals[TRIANGLE_COUNT * 3];

	mf::RandomGenerator randomGenerator;
	randomGenerator.seedGenerator();
	mf::distributeTriangles(randomGenerator, 2.0f, TRIANGLE_COUNT, glm::vec2(0.2f, 0.3f), vertexPositions, vertexNormals);

	// Create vertex buffer
	positionBuffer = new VertexBufferVulkan(handle, TRIANGLE_COUNT * 3 * sizeof(glm::vec4), VertexBufferVulkan::DATA_USAGE::STATIC);
	positionBufferBinding = VertexBufferVulkan::Binding(positionBuffer, sizeof(glm::vec4), TRIANGLE_COUNT * 3, 0);
	positionBuffer->setData(vertexPositions, positionBufferBinding);

	normalBuffer = new VertexBufferVulkan(handle, TRIANGLE_COUNT * 3 * sizeof(glm::vec3), VertexBufferVulkan::DATA_USAGE::STATIC);
	normalBufferBinding = VertexBufferVulkan::Binding(normalBuffer, sizeof(glm::vec3), TRIANGLE_COUNT * 3, 0);
	normalBuffer->setData(vertexNormals, normalBufferBinding);

	// Create shaders
	depthPassShaders = new ShaderVulkan("depthPassShaders", handle);
	depthPassShaders->setShader("resource/Shadow/depthPass/???", ShaderVulkan::ShaderType::VS);	// todo: create shaders and fill in their names
	depthPassShaders->setShader("resource/Shadow/depthPass/???", ShaderVulkan::ShaderType::PS);
	std::string err;
	depthPassShaders->compileMaterial(err);

	renderPassShaders = new ShaderVulkan("renderPassShaders", handle);
	renderPassShaders->setShader("resource/Shadow/renderPass/???", ShaderVulkan::ShaderType::VS);	// todo: create shaders and fill in their names
	renderPassShaders->setShader("resource/Shadow/renderPass/???", ShaderVulkan::ShaderType::PS);
	renderPassShaders->compileMaterial(err);

	// Create image and sampler
	shadowMapSampler = new Sampler2DVulkan(handle);
	shadowMapSampler->setMinFilter(VkFilter::VK_FILTER_LINEAR);
	shadowMapSampler->setMagFilter(VkFilter::VK_FILTER_LINEAR);
	shadowMapSampler->setWrap(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);

	shadowMap = new Texture2DVulkan(handle, shadowMapSampler);
	shadowMap->createShadowMap(shadowMapSize, shadowMapSize, shadowMapFormat);

	// Create frame buffer for shadow map
	VkFramebufferCreateInfo framebufferCreateInfo = {};
	framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferCreateInfo.pNext = nullptr;
	framebufferCreateInfo.flags = 0;
	framebufferCreateInfo.renderPass = handle->getFramePass();
	framebufferCreateInfo.attachmentCount = 1;
	framebufferCreateInfo.pAttachments = &(shadowMap->imageInfo.imageView);
	framebufferCreateInfo.width = shadowMapSize;
	framebufferCreateInfo.height = shadowMapSize;
	framebufferCreateInfo.layers = 1;

	if (vkCreateFramebuffer(handle->getDevice(), &framebufferCreateInfo, nullptr, &shadowMapFrameBuffer) != VK_SUCCESS)
		throw std::runtime_error("Failed to create frame buffer");

	// Create techniques
	const uint32_t BUFFER_COUNT = 2;
	const uint32_t ATTRIBUTE_COUNT = 2;
	VkVertexInputBindingDescription vertexBufferBindings[BUFFER_COUNT] =
	{
		defineVertexBinding(0, 16),
		defineVertexBinding(1, 12)
	};
	VkVertexInputAttributeDescription vertexAttributes[ATTRIBUTE_COUNT] =
	{
		defineVertexAttribute(0, 0, VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT, 0),
		defineVertexAttribute(1, 1, VkFormat::VK_FORMAT_R32G32B32_SFLOAT, 0)
	};
	VkPipelineVertexInputStateCreateInfo vertexBindings =
		defineVertexBufferBindings(vertexBufferBindings, BUFFER_COUNT, vertexAttributes, ATTRIBUTE_COUNT);

	depthPassTechnique = new TechniqueVulkan(_renderHandle, depthPassShaders, _renderHandle->getFramePass(), vertexBindings);
	renderPassTechnique = new TechniqueVulkan(_renderHandle, renderPassShaders, _renderHandle->getFramePass(), vertexBindings);

	// Define viewport
	shadowMapViewport.x = 0;
	shadowMapViewport.y = 0;
	shadowMapViewport.height = shadowMapSize;
	shadowMapViewport.width = shadowMapSize;
	shadowMapViewport.minDepth = 0.0f;
	shadowMapViewport.maxDepth = 1.0f;

	// Create descriptors and descriptor sets
	createDescriptors();
}


void ShadowScene::frame(VkCommandBuffer cmdBuf)
{
	_renderHandle->beginFramePass(&shadowMapFrameBuffer);

	// Shadow map pass
	vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, depthPassTechnique->pipeline);

	vkCmdSetViewport(cmdBuf, 0, 1, &shadowMapViewport);

	positionBufferBinding.bind(0);
	normalBufferBinding.bind(0);
	vkCmdDraw(cmdBuf, positionBufferBinding.numElements, 1, 0, 0);


	// Rendering pass
	vkCmdNextSubpass(cmdBuf, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport normalViewport = _renderHandle->getViewport();
	vkCmdSetViewport(cmdBuf, 0, 1, &normalViewport);

	// todo: bind descriptors

	vkCmdDraw(cmdBuf, positionBufferBinding.numElements, 1, 0, 0);

	_renderHandle->submitFramePass();
}


void ShadowScene::defineDescriptorLayout(VkDevice device, std::vector<VkDescriptorSetLayout>& layout)
{
	layout.resize(2);
	VkDescriptorSetLayoutBinding depthPassBinding;
	VkDescriptorSetLayoutBinding renderPassBindings[3];

	writeLayoutBinding(depthPassBinding, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);	// Transform matrix for light

	writeLayoutBinding(renderPassBindings[0], shadowMapBindingSlot, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);	// Shadow map
	writeLayoutBinding(renderPassBindings[1], lightMatrixBindingSlot, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);			// Light transform matrix
	writeLayoutBinding(renderPassBindings[2], cameraMatrixBindingSlot, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);			// Camera transform matrix

	layout[0] = createDescriptorLayout(device, &depthPassBinding, 1);
	layout[1] = createDescriptorLayout(device, &renderPassBindings[0], 3);
}


VkRenderPass ShadowScene::defineRenderPass(VkDevice device, VkFormat swapchainFormat, VkFormat depthFormat)
{
	VkRenderPass renderPass;

	const uint32_t ATTATCHMENT_COUNT = 3;
	VkAttachmentDescription attatchments[ATTATCHMENT_COUNT] =
	{
		defineFramebufColor(swapchainFormat),
		defineFramebufDepth(depthFormat),
		//defineFramebufShadowMap(shadowMapFormat)
	};

	VkAttachmentReference colorRef = {};
	colorRef.attachment = 0;
	colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthRef = {};
	depthRef.attachment = 1;
	depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference shadowMapWriteRef = {};
	shadowMapWriteRef.attachment = 2;
	shadowMapWriteRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	const uint32_t SUBPASS_COUNT = 2;
	VkSubpassDescription subpasses[SUBPASS_COUNT];
	subpasses[0].flags = 0;
	subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpasses[0].inputAttachmentCount = 0;
	subpasses[0].pInputAttachments = nullptr;
	subpasses[0].colorAttachmentCount = 0;
	subpasses[0].pColorAttachments = nullptr;
	subpasses[0].pResolveAttachments = nullptr;
	subpasses[0].pDepthStencilAttachment = &shadowMapWriteRef;
	subpasses[0].preserveAttachmentCount = 0;
	subpasses[0].pPreserveAttachments = nullptr;

	subpasses[1].flags = 0;
	subpasses[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpasses[1].inputAttachmentCount = 0;
	subpasses[1].pInputAttachments = nullptr;
	subpasses[1].colorAttachmentCount = 1;
	subpasses[1].pColorAttachments = &colorRef;
	subpasses[1].pResolveAttachments = nullptr;
	subpasses[1].pDepthStencilAttachment = &depthRef;
	subpasses[1].preserveAttachmentCount = 0;
	subpasses[1].pPreserveAttachments = nullptr;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = 0;
	dependency.dstSubpass = 1;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependency.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT; // VK_ACCESS_SHADER_READ_BIT?
	dependency.dependencyFlags = 0;

	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.pNext = nullptr;
	renderPassCreateInfo.flags = 0;
	renderPassCreateInfo.attachmentCount = 3;
	renderPassCreateInfo.pAttachments = attatchments;
	renderPassCreateInfo.subpassCount = 2;
	renderPassCreateInfo.pSubpasses = subpasses;
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass) != VK_SUCCESS)
		throw std::runtime_error("Failed to create shadow render pass");

	return renderPass;
}

void ShadowScene::createDescriptors()
{
	// todo
}
