#include "Scenes/ShadowScene.h"
#include "VulkanRenderer.h"
#include "Stuff/RandomGenerator.h"
#include "VertexBufferVulkan.h"

#include "glm\gtc\matrix_transform.hpp"
#define OBJ_READER_SIMPLE
#include "Stuff/ObjReaderSimple.h"

ShadowScene::ShadowScene()
{
	glm::mat4 lightMatrix = glm::mat4(1.0f);
	lightMatrix = rotationMatrix(glm::pi<float>() * 0.0f, glm::vec3(0, 1, 0)) * lightMatrix;
	lightMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -4.0f)) * lightMatrix;
	lightMatrix = orthographicMatrix(-4.0f, 4.0f, -4.0f, 4.0f, 0.1f, 10.0f) * lightMatrix;

	createCameraMatrix(0.0f);
	shadowMappingMatrix = lightMatrix;
	lightInfo.clipSpaceToShadowMapMatrix = lightMatrix;
}

ShadowScene::~ShadowScene()
{
	delete shadowMappingMatrixBuffer;
	delete transformMatrixBuffer;
	delete lightInfoBuffer;

	delete positionBuffer;
	delete normalBuffer;

	delete shadowMapSampler;
	delete shadowMap;

	delete depthPassTechnique;
	delete depthPassShaders;

	delete renderPassTechnique;
	delete renderPassShaders;
	VkDevice dev = _renderHandle->getDevice();

	vkDestroyFramebuffer(dev, shadowFramebuffer, nullptr);
	vkDestroyRenderPass(dev, shadowRenderPass, nullptr);
	vkDestroyDescriptorPool(_renderHandle->getDevice(), desciptorPool, nullptr);

	pipelineLayoutConstruct.destroy(_renderHandle->getDevice());

	//Post
	delete techniqueBlurHorizontal, delete techniqueBlurVertical;
	delete blurHorizontal, delete blurVertical;
	postLayout.destroy(_renderHandle->getDevice());
}

void ShadowScene::initialize(VulkanRenderer* handle)
{
	Scene::initialize(handle);



	// Create vertex buffer
	if (false)
	{
		// Create triangles
		const uint32_t TRIANGLE_COUNT = 100;
		glm::vec4 vertexPositions[TRIANGLE_COUNT * 3];
		glm::vec3 vertexNormals[TRIANGLE_COUNT * 3];

		mf::RandomGenerator randomGenerator;
		randomGenerator.seedGenerator();
		//randomGenerator.setSeed({ 2 });
		mf::distributeTriangles(randomGenerator, 2.0f, TRIANGLE_COUNT, glm::vec2(0.6f, 0.8f), vertexPositions, vertexNormals);

		//Create buffers
		positionBuffer = new VertexBufferVulkan(handle, TRIANGLE_COUNT * 3 * sizeof(glm::vec4), VertexBufferVulkan::DATA_USAGE::STATIC);
		positionBufferBinding = VertexBufferVulkan::Binding(positionBuffer, sizeof(glm::vec4), TRIANGLE_COUNT * 3, 0);
		normalBuffer = new VertexBufferVulkan(handle, TRIANGLE_COUNT * 3 * sizeof(glm::vec3), VertexBufferVulkan::DATA_USAGE::STATIC);
		normalBufferBinding = VertexBufferVulkan::Binding(normalBuffer, sizeof(glm::vec3), TRIANGLE_COUNT * 3, 0);

		positionBuffer->setData(vertexPositions, positionBufferBinding);
		normalBuffer->setData(vertexNormals, normalBufferBinding);
	}
	else
	{
		SimpleMesh mesh, baked;
		if (readObj("resource/Suzanne.obj", mesh))
			std::cout << "Obj read successfull\n";
		mesh.bake(SimpleMesh::BitFlag::NORMAL_BIT | SimpleMesh::TRIANGLE_ARRAY | SimpleMesh::POS_4_COMPONENT, baked);

		size_t num_tri = baked._position.size() / 4;
		positionBuffer = new VertexBufferVulkan(handle, num_tri * sizeof(glm::vec4), VertexBufferVulkan::DATA_USAGE::STATIC);
		positionBufferBinding = VertexBufferVulkan::Binding(positionBuffer, sizeof(glm::vec4), num_tri, 0);
		normalBuffer = new VertexBufferVulkan(handle, num_tri * sizeof(glm::vec3), VertexBufferVulkan::DATA_USAGE::STATIC);
		normalBufferBinding = VertexBufferVulkan::Binding(normalBuffer, sizeof(glm::vec3), num_tri, 0);

		positionBuffer->setData(baked._position.data(), positionBufferBinding);
		normalBuffer->setData(baked._normal.data(), normalBufferBinding);
	}

	// Create shaders
	depthPassShaders = new ShaderVulkan("depthPassShaders", handle);
	depthPassShaders->setShader("resource/Shadow/depthPass/ShadowPassVertexShader.glsl", ShaderVulkan::ShaderType::VS);
	std::string err;
	depthPassShaders->compileMaterial(err);

	renderPassShaders = new ShaderVulkan("renderPassShaders", handle);
	renderPassShaders->setShader("resource/Shadow/renderPass/VertexShader.glsl", ShaderVulkan::ShaderType::VS);
	renderPassShaders->setShader("resource/Shadow/renderPass/FragmentShader.glsl", ShaderVulkan::ShaderType::PS);
	renderPassShaders->compileMaterial(err);

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

	depthPassTechnique = new TechniqueVulkan(_renderHandle, depthPassShaders, shadowRenderPass, _renderHandle->getFramePassLayout(), vertexBindings, 0);
	renderPassTechnique = new TechniqueVulkan(_renderHandle, renderPassShaders, _renderHandle->getFramePass(), _renderHandle->getFramePassLayout(), vertexBindings, 0);

	// Define viewport
	shadowMapViewport.x = 0;
	shadowMapViewport.y = 0;
	shadowMapViewport.height = (float)shadowMapSize;
	shadowMapViewport.width = (float)shadowMapSize;
	shadowMapViewport.minDepth = 0.0f;
	shadowMapViewport.maxDepth = 1.0f;

	createBuffers();

	// Create descriptor pool
	VkDescriptorPoolSize poolSizes[2];
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 3;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = 1;
	desciptorPool = createDescriptorPool(_renderHandle->getDevice(), poolSizes, 2, 4);

	// Allocate descriptor sets
	VkDescriptorSetAllocateInfo shadowPassAllocateInfo = {};
	shadowPassAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	shadowPassAllocateInfo.pNext = nullptr;
	shadowPassAllocateInfo.descriptorPool = desciptorPool;
	shadowPassAllocateInfo.descriptorSetCount = 1;
	VkDescriptorSetLayout descriptorSetLayout = _renderHandle->getDescriptorSetLayout(0);
	shadowPassAllocateInfo.pSetLayouts = &descriptorSetLayout;
	vkAllocateDescriptorSets(_renderHandle->getDevice(), &shadowPassAllocateInfo, &shadowPassDescriptorSet);

	VkDescriptorSetAllocateInfo renderPassAllocateInfo = {};
	renderPassAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	renderPassAllocateInfo.pNext = nullptr;
	renderPassAllocateInfo.descriptorPool = desciptorPool;
	renderPassAllocateInfo.descriptorSetCount = 1;
	descriptorSetLayout = _renderHandle->getDescriptorSetLayout(1);
	renderPassAllocateInfo.pSetLayouts = &descriptorSetLayout;
	vkAllocateDescriptorSets(_renderHandle->getDevice(), &renderPassAllocateInfo, &renderPassDescriptorSet);

	// Shadow pass layout
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	bindings.resize(1);
	pipelineLayoutConstruct = vk::LayoutConstruct(2);
	writeLayoutBinding(bindings[0], 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	pipelineLayoutConstruct[0] = createDescriptorLayout(_renderHandle->getDevice(), &bindings[0], 1);

	// Render pass layout
	std::vector<DescriptorInfo> renderPassDescriptorInfos;
	renderPassDescriptorInfos.push_back(DescriptorInfo
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, shadowMapBindingSlot, VK_SHADER_STAGE_FRAGMENT_BIT });

	renderPassDescriptorInfos.push_back(DescriptorInfo
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, transformMatrixBindingSlot, VK_SHADER_STAGE_VERTEX_BIT });

	renderPassDescriptorInfos.push_back(DescriptorInfo
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, clipToShadowMapMatrixBindingSlot, VK_SHADER_STAGE_FRAGMENT_BIT });

	bindings = generateDescriptorSetLayoutBinding(renderPassDescriptorInfos);
	pipelineLayoutConstruct[1] = createDescriptorLayout(_renderHandle->getDevice(), &bindings[0], 3);

	pipelineLayoutConstruct.construct(_renderHandle->getDevice());

	// Write descriptors into descriptor sets
	// Shadow pass
	VkDescriptorBufferInfo shadowMatrixInfo = {};
	shadowMatrixInfo.buffer = shadowMappingMatrixBuffer->getBuffer();
	shadowMatrixInfo.offset = 0;
	shadowMatrixInfo.range = VK_WHOLE_SIZE;

	VkWriteDescriptorSet shadowPassWrite = {};
	shadowPassWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	shadowPassWrite.pNext = nullptr;
	shadowPassWrite.dstSet = shadowPassDescriptorSet;
	shadowPassWrite.dstBinding = shadowMappingMatrixBindingSlot;
	shadowPassWrite.descriptorCount = 1;
	shadowPassWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	shadowPassWrite.pBufferInfo = &shadowMatrixInfo;

	vkUpdateDescriptorSets(_renderHandle->getDevice(), 1, &shadowPassWrite, 0, nullptr);

	// Render pass
	VkWriteDescriptorSet renderPassInfoWrites[3] = { {}, {}, {} };	// This initialization is important, vkUpdateDescriptorSets will freeze without it

	VkDescriptorImageInfo shadowMapInfo = {};
	shadowMapInfo.sampler = shadowMapSampler->_sampler;
	shadowMapInfo.imageView = shadowMap->imageInfo.imageView;
	shadowMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	renderPassInfoWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	renderPassInfoWrites[0].pNext = nullptr;
	renderPassInfoWrites[0].dstSet = renderPassDescriptorSet;
	renderPassInfoWrites[0].dstBinding = shadowMapBindingSlot;
	renderPassInfoWrites[0].descriptorCount = 1;
	renderPassInfoWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	renderPassInfoWrites[0].pImageInfo = &shadowMapInfo;

	VkDescriptorBufferInfo transformMatrixInfo = {};
	transformMatrixInfo.buffer = transformMatrixBuffer->getBuffer();
	transformMatrixInfo.offset = 0;
	transformMatrixInfo.range = VK_WHOLE_SIZE;

	renderPassInfoWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	renderPassInfoWrites[1].pNext = nullptr;
	renderPassInfoWrites[1].dstSet = renderPassDescriptorSet;
	renderPassInfoWrites[1].dstBinding = transformMatrixBindingSlot;
	renderPassInfoWrites[1].descriptorCount = 1;
	renderPassInfoWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	renderPassInfoWrites[1].pBufferInfo = &transformMatrixInfo;

	VkDescriptorBufferInfo clipSpaceToShadowMapMatrixInfo = {};
	clipSpaceToShadowMapMatrixInfo.buffer = lightInfoBuffer->getBuffer();
	clipSpaceToShadowMapMatrixInfo.offset = 0;
	clipSpaceToShadowMapMatrixInfo.range = VK_WHOLE_SIZE;

	renderPassInfoWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	renderPassInfoWrites[2].pNext = nullptr;
	renderPassInfoWrites[2].dstSet = renderPassDescriptorSet;
	renderPassInfoWrites[2].dstBinding = clipToShadowMapMatrixBindingSlot;
	renderPassInfoWrites[2].descriptorCount = 1;
	renderPassInfoWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	renderPassInfoWrites[2].pBufferInfo = &clipSpaceToShadowMapMatrixInfo;

	vkUpdateDescriptorSets(_renderHandle->getDevice(), 3, &renderPassInfoWrites[0], 0, nullptr);


	// Post pass initiation

	//Layout
	VkDescriptorSetLayoutBinding binding;
	postLayout = vk::LayoutConstruct(2);
	writeLayoutBinding(binding, 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
	postLayout[0] = createDescriptorLayout(_renderHandle->getDevice(), &binding, 1);
	writeLayoutBinding(binding, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT);
	postLayout[1] = createDescriptorLayout(_renderHandle->getDevice(), &binding, 1);
	// Gen. layout
	postLayout.construct(_renderHandle->getDevice());

	// Gen. shaders
	blurHorizontal = new ShaderVulkan("CopyCompute", _renderHandle);
	blurHorizontal->setShader("resource/Compute/GaussianHorizontal.glsl", ShaderVulkan::ShaderType::CS);
	blurHorizontal->compileMaterial(err);

	blurVertical = new ShaderVulkan("CopyCompute", _renderHandle);
	blurVertical->setShader("resource/Compute/GaussianVertical.glsl", ShaderVulkan::ShaderType::CS);
	blurVertical->compileMaterial(err);

	// Gen techniques
	techniqueBlurHorizontal = new TechniqueVulkan(_renderHandle, blurHorizontal, postLayout._layout);
	techniqueBlurVertical = new TechniqueVulkan(_renderHandle, blurVertical, postLayout._layout);

	// Frame buffer image bindings
	swapChainImgDesc.resize(_renderHandle->getSwapChainLength());
	VkDescriptorImageInfo imgInfo[5];
	VkWriteDescriptorSet writeInfo[5];
	for (size_t i = 0; i < swapChainImgDesc.size(); i++)
	{
		imgInfo[i].sampler = NULL;
		imgInfo[i].imageView = _renderHandle->getSwapChainView(i);
		imgInfo[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		swapChainImgDesc[i] = _renderHandle->generateDescriptor(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, postLayout._desc);
		writeDescriptorStruct_IMG_STORAGE(writeInfo[i], swapChainImgDesc[i], 0, 0, 1, imgInfo + i);
	}
	vkUpdateDescriptorSets(_renderHandle->getDevice(), (uint32_t)swapChainImgDesc.size(), writeInfo, 0, nullptr);
}

void ShadowScene::transfer()
{

}

void ShadowScene::frame(float dt)
{
	static float counter = 0.0f;
	counter += dt;
	createCameraMatrix(counter);

	VkCommandBuffer cmdBuf = beginSingleCommand(_renderHandle->getDevice(), _renderHandle->queues[QueueType::MEM].pool);
	transformMatrixBuffer->setData(&transformMatrix, sizeof(glm::mat4), 1);
	endSingleCommand_Wait(_renderHandle->getDevice(), _renderHandle->queues[QueueType::MEM].queue, _renderHandle->queues[QueueType::MEM].pool, cmdBuf);

	cmdBuf = beginSingleCommand(_renderHandle->getDevice(), _renderHandle->queues[QueueType::MEM].pool);
	lightInfoBuffer->setData(&lightInfo, sizeof(lightInfo), 1);
	endSingleCommand_Wait(_renderHandle->getDevice(), _renderHandle->queues[QueueType::MEM].queue, _renderHandle->queues[QueueType::MEM].pool, cmdBuf);


	VulkanRenderer::FrameInfo info = _renderHandle->beginCommandBuffer();

	// Shadow map pass
	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = shadowRenderPass;
	renderPassInfo.framebuffer = shadowFramebuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent.height = shadowMapSize;
	renderPassInfo.renderArea.extent.width = shadowMapSize;
	// Clear params
	VkClearValue clearValue;
	clearValue.depthStencil = { 1.0f, 0 };
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearValue;

	vkCmdBeginRenderPass(info._buf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(info._buf, VK_PIPELINE_BIND_POINT_GRAPHICS, depthPassTechnique->pipeline);
	vkCmdSetViewport(info._buf, 0, 1, &shadowMapViewport);
	vkCmdBindDescriptorSets(info._buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayoutConstruct._layout, 0, 1, &shadowPassDescriptorSet, 0, nullptr);

	positionBufferBinding.bind(info._buf, 0);
	normalBufferBinding.bind(info._buf, 1);
	vkCmdDraw(info._buf, positionBufferBinding.numElements, 1, 0, 0);
	vkCmdEndRenderPass(info._buf);

	// Image barrier transferring image layout
	transition_DepthRead(info._buf, shadowMap->_imageHandle);
	
	// Rendering pass
	_renderHandle->beginRenderPass(info._buf);
	vkCmdBindPipeline(info._buf, VK_PIPELINE_BIND_POINT_GRAPHICS, renderPassTechnique->pipeline);
	VkViewport normalViewport = _renderHandle->getViewport();
	vkCmdSetViewport(info._buf, 0, 1, &normalViewport);
	vkCmdBindDescriptorSets(info._buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayoutConstruct._layout, 1, 1, &renderPassDescriptorSet, 0, nullptr);

	vkCmdDraw(info._buf, positionBufferBinding.numElements, 1, 0, 0);

	_renderHandle->endRenderPass();
	// Submit
	transition_DepthWrite(info._buf, shadowMap->_imageHandle);
	_renderHandle->submitFramePass();

	post();

	_renderHandle->present();
}


void ShadowScene::post()
{

	VulkanRenderer::FrameInfo info = _renderHandle->beginCompute();
	transition_RenderToPost(info._buf, info._swapChainImage, _renderHandle->getQueueFamily(QueueType::GRAPHIC), _renderHandle->getQueueFamily(QueueType::COMPUTE));


	// Bind compute shader
	techniqueBlurHorizontal->bind(info._buf, VK_PIPELINE_BIND_POINT_COMPUTE);
	vkCmdBindDescriptorSets(info._buf, VK_PIPELINE_BIND_POINT_COMPUTE, postLayout._layout, 0, 1, &swapChainImgDesc[info._swapChainIndex], 0, nullptr);
	// Dispatch
	vkCmdDispatch(info._buf, 1, 512, 1);


	// Bind compute shader
	techniqueBlurVertical->bind(info._buf, VK_PIPELINE_BIND_POINT_COMPUTE);
	// Dispatch
	vkCmdDispatch(info._buf, 1, 512, 1);

	// Finish
	transition_PostToPresent(info._buf, info._swapChainImage, _renderHandle->getQueueFamily(QueueType::COMPUTE), _renderHandle->getQueueFamily(QueueType::GRAPHIC));
	_renderHandle->submitCompute();
}


void ShadowScene::defineDescriptorLayout(VkDevice device, std::vector<VkDescriptorSetLayout>& layout)
{
	layout.resize(2);
	VkDescriptorSetLayoutBinding depthPassBinding;
	VkDescriptorSetLayoutBinding renderPassBindings[3];

	// shadowMappingMatrix
	writeLayoutBinding(depthPassBinding, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);

	// Shadow map
	writeLayoutBinding(renderPassBindings[0], shadowMapBindingSlot, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

	// transformMatrix
	writeLayoutBinding(renderPassBindings[1], transformMatrixBindingSlot, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	
	// clipSpaceToShadowMapMatrix
	writeLayoutBinding(renderPassBindings[2], clipToShadowMapMatrixBindingSlot, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);

	layout[0] = createDescriptorLayout(device, &depthPassBinding, 1);
	layout[1] = createDescriptorLayout(device, &renderPassBindings[0], 3);
}


VkRenderPass ShadowScene::defineRenderPass(VkDevice device, VkFormat swapchainFormat, VkFormat depthFormat, std::vector<VkImageView>& additionalAttatchments)
{
	// Create image and sampler
	// Shouldn't really be done here, but the renderer needs the shadow map image view at this point
	shadowMapSampler = new Sampler2DVulkan(_renderHandle);
	shadowMapSampler->setMinFilter(VkFilter::VK_FILTER_LINEAR);
	shadowMapSampler->setMagFilter(VkFilter::VK_FILTER_LINEAR);
	shadowMapSampler->setWrap(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);

	shadowMap = new Texture2DVulkan(_renderHandle, shadowMapSampler);
	shadowMap->createShadowMap(shadowMapSize, shadowMapSize, shadowMapFormat);

	// Define shadow render pass
	defineShadowRenderPass(device);

	// Define the render pass
	VkRenderPass renderPass;

	const uint32_t ATTATCHMENT_COUNT = 2;
	VkAttachmentDescription attatchments[ATTATCHMENT_COUNT] =
	{
		defineFramebufColor(swapchainFormat),
		defineFramebufDepth(depthFormat)
	};

	VkAttachmentReference colorRef = {};
	colorRef.attachment = 0;
	colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthRef = {};
	depthRef.attachment = 1;
	depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass;
	subpass.flags = 0;
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = nullptr;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorRef;
	subpass.pResolveAttachments = nullptr;
	subpass.pDepthStencilAttachment = &depthRef;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = nullptr;

	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.pNext = nullptr;
	renderPassCreateInfo.flags = 0;
	renderPassCreateInfo.attachmentCount = ATTATCHMENT_COUNT;
	renderPassCreateInfo.pAttachments = attatchments;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = 0;
	renderPassCreateInfo.pDependencies = nullptr;

	if (vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass) != VK_SUCCESS)
		throw std::runtime_error("Failed to create shadow render pass");

	return renderPass;
}

void ShadowScene::defineShadowRenderPass(VkDevice device)
{
	VkAttachmentDescription attatchment = defineFramebufShadowMap(shadowMapFormat);

	VkAttachmentReference shadowMapWriteRef = {};
	shadowMapWriteRef.attachment = 0;
	shadowMapWriteRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass;
	subpass.flags = 0;
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = nullptr;
	subpass.colorAttachmentCount = 0;
	subpass.pColorAttachments = nullptr;
	subpass.pResolveAttachments = nullptr;
	subpass.pDepthStencilAttachment = &shadowMapWriteRef;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = nullptr;

	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.pNext = nullptr;
	renderPassCreateInfo.flags = 0;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &attatchment;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = 0;
	renderPassCreateInfo.pDependencies = nullptr;

	if (vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &shadowRenderPass) != VK_SUCCESS)
		throw std::runtime_error("Failed to create shadow render pass");

	VkFramebufferCreateInfo shadowFramebufferCreateInfo = {};
	shadowFramebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	shadowFramebufferCreateInfo.pNext = nullptr;
	shadowFramebufferCreateInfo.flags = 0;
	shadowFramebufferCreateInfo.renderPass = shadowRenderPass;
	shadowFramebufferCreateInfo.attachmentCount = 1;
	shadowFramebufferCreateInfo.pAttachments = &(shadowMap->imageInfo.imageView);
	shadowFramebufferCreateInfo.width = shadowMapSize;
	shadowFramebufferCreateInfo.height = shadowMapSize;
	shadowFramebufferCreateInfo.layers = 1;

	if (vkCreateFramebuffer(device, &shadowFramebufferCreateInfo, nullptr, &shadowFramebuffer) != VK_SUCCESS)
		throw std::runtime_error("Failed to create shadow framebuffer");
}

void ShadowScene::createBuffers()
{
	shadowMappingMatrixBuffer = new ConstantBufferVulkan(_renderHandle);
	shadowMappingMatrixBuffer->setData(&shadowMappingMatrix, sizeof(glm::mat4), 0);

	transformMatrixBuffer = new ConstantBufferVulkan(_renderHandle);
	transformMatrixBuffer->setUseCustomDescriptor(true);
	transformMatrixBuffer->setData(&transformMatrix, sizeof(glm::mat4), 1);

	lightInfoBuffer = new ConstantBufferVulkan(_renderHandle);
	lightInfoBuffer->setUseCustomDescriptor(true);
	lightInfoBuffer->setData(&lightInfo, sizeof(lightInfo), 1);
}

glm::mat4 ShadowScene::rotationMatrix(float angle, glm::vec3 const& axis)
{
	float x;
	float y;
	float z;

	glm::vec3 normalized = glm::normalize(axis);
	x = normalized[0];
	y = normalized[1];
	z = normalized[2];


	const float c = cosf(angle);
	const float _1_c = 1.0f - c;
	const float s = sinf(angle);

	glm::mat4 rotation = {
		x * x * _1_c + c,
		y * x * _1_c - z * s,
		z * x * _1_c + y * s,
		0.0f,

		x * y * _1_c + z * s,
		y * y * _1_c + c,
		z * y * _1_c - x * s,
		0.0f,

		x * z * _1_c - y * s,
		y * z * _1_c + x * s,
		z * z * _1_c + c,
		0.0f,

		0.0f,
		0.0f,
		0.0f,
		1.0f
	};

	return rotation;
}

glm::mat4 ShadowScene::orthographicMatrix(float left, float right, float bottom, float top, float near, float far)
{
	glm::mat4 orthographic = {
		2.0f / (right - left),
		0.0f,
		0.0f,
		0.0f,

		0.0f,
		2.0f / (bottom - top),
		0.0f,
		0.0f,

		0.0f,
		0.0f,
		1.0f / (near - far),
		0.0f,

		-(right + left) / (right - left),
		-(bottom + top) / (bottom - top),
		near / (near - far),
		1.0f
	};
	return orthographic;
}

glm::mat4 ShadowScene::perspectiveMatrix(float aspectRatio, float fov, float near, float far)
{
	float f = 1.0f / tan(0.5f * fov);

	glm::mat4 perspective = {
		f / aspectRatio,
		0.0f,
		0.0f,
		0.0f,

		0.0f,
		-f,
		0.0f,
		0.0f,

		0.0f,
		0.0f,
		far / (near - far),
		-1.0f,

		0.0f,
		0.0f,
		(near * far) / (near - far),
		0.0f
	};
	return perspective;
}

void ShadowScene::createCameraMatrix(float time)
{
	glm::mat4 cameraMatrix = glm::mat4(1.0f);
	glm::mat4 rot = rotationMatrix(glm::pi<float>() * 0.2f * sinf(time), glm::vec3(.0f, 1.0f, .0f));
	glm::mat4 tra = glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f * sinf(time), 0.5f * sinf(time * 0.1f), -4.0f));
	glm::mat4 per = perspectiveMatrix(800.0f / 600.0f, 1.0f, 0.1f, 10.0f);
	//glm::mat4 per = orthographicMatrix(-4.0f, 4.0f, -4.0f, 4.0f, 0.1f, 10.0f) * cameraMatrix;

	transformMatrix = per * tra * rot * cameraMatrix;
	lightInfo.lightDirection = glm::inverse(rot) * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
}
