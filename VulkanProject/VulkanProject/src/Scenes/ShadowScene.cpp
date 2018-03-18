#include "Scenes/ShadowScene.h"
#include "VulkanRenderer.h"
#include "Stuff/RandomGenerator.h"
#include "VertexBufferVulkan.h"

#include "glm\gtc\matrix_transform.hpp"
#define OBJ_READER_SIMPLE
#include "Stuff/ObjReaderSimple.h"

ShadowScene::ShadowScene(FrameType frameType)
{
	this->frameType = frameType;
	firstFrame = true;
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

	shadowPipeLayout.destroy(_renderHandle->getDevice());

	//Post
	delete techniqueBlurHorizontal, delete techniqueBlurVertical;
	delete blurHorizontal, delete blurVertical;
	postLayout.destroy(_renderHandle->getDevice());
}

//#define COMPILE
void ShadowScene::initialize(VulkanRenderer* handle)
{
	Scene::initialize(handle);
	// Define shadow render pass
	defineShadowRenderPass(handle->getDevice());

	glm::mat4 lightMatrix = glm::mat4(1.0f);
	lightMatrix = rotationMatrix(glm::pi<float>() * 0.0f, glm::vec3(0, 1, 0)) * lightMatrix;
	lightMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -20.0f)) * lightMatrix;
	lightMatrix = orthographicMatrix(-20.0f, 20.0f, -20.0f, 20.0f, 0.1f, 60.0f) * lightMatrix;

	createCameraMatrix(0.0f);
	shadowMappingMatrix = lightMatrix;
	lightInfo.clipSpaceToShadowMapMatrix = lightMatrix;

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
		if (readObj("resource/cowparty.obj", mesh))
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
	renderPassShaders = new ShaderVulkan("renderPassShaders", handle);
#ifdef COMPILE
	depthPassShaders->setShader("resource/Shadow/depthPass/ShadowPassVertexShader.glsl", ShaderVulkan::ShaderType::VS);
	renderPassShaders->setShader("resource/Shadow/renderPass/VertexShader.glsl", ShaderVulkan::ShaderType::VS);
	renderPassShaders->setShader("resource/Shadow/renderPass/FragmentShader.glsl", ShaderVulkan::ShaderType::PS);
#else
	depthPassShaders->setShader("resource/tmp/ShadowPassVertexShader.spv", ShaderVulkan::ShaderType::VS);
	renderPassShaders->setShader("resource/tmp/VertexShader.spv", ShaderVulkan::ShaderType::VS);
	renderPassShaders->setShader("resource/tmp/FragmentShader.spv", ShaderVulkan::ShaderType::PS);
#endif
	std::string err;
	depthPassShaders->compileMaterial(err);
	renderPassShaders->compileMaterial(err);

	shadowMap->attachBindPoint(1, _renderHandle->getDescriptorSetLayout(1));

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

	renderPassTechnique = new TechniqueVulkan(_renderHandle, renderPassShaders, _renderHandle->getFramePass(), _renderHandle->getFramePassLayout(), vertexBindings);
	depthPassTechnique = new TechniqueVulkan(_renderHandle, depthPassShaders, shadowRenderPass, shadowPipeLayout._layout, vertexBindings);

	// Define viewport
	shadowMapViewport.x = 0;
	shadowMapViewport.y = 0;
	shadowMapViewport.height = (float)shadowMapSize;
	shadowMapViewport.width = (float)shadowMapSize;
	shadowMapViewport.minDepth = 0.0f;
	shadowMapViewport.maxDepth = 1.0f;

	createBuffers();

	// Post pass initiation

	//Layout
	postLayout = vk::LayoutConstruct(2);
	VkDescriptorSetLayoutBinding binding;
	writeLayoutBinding(binding, 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
	postLayout[0] = createDescriptorLayout(_renderHandle->getDevice(), &binding, 1);
	writeLayoutBinding(binding, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT);
	postLayout[1] = createDescriptorLayout(_renderHandle->getDevice(), &binding, 1);
	// Gen. layout
	postLayout.construct(_renderHandle->getDevice());

	// Gen. shaders
	blurHorizontal = new ShaderVulkan("HorizontalGauss", _renderHandle);
	blurVertical = new ShaderVulkan("VerticalGauss", _renderHandle);
#ifdef COMPILE
	blurHorizontal->setShader("resource/Compute/GaussianHorizontal.glsl", ShaderVulkan::ShaderType::CS);
	blurVertical->setShader("resource/Compute/GaussianVertical.glsl", ShaderVulkan::ShaderType::CS);
#else
	blurHorizontal->setShader("resource/tmp/GaussianHorizontal.spv", ShaderVulkan::ShaderType::CS);
	blurVertical->setShader("resource/tmp/GaussianVertical.spv", ShaderVulkan::ShaderType::CS);
#endif

	blurHorizontal->compileMaterial(err);
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

	VkDevice device = _renderHandle->getDevice();

	depthCommandBuf[0] = allocateCmdBuf(device, _renderHandle->queues[QueueType::GRAPHIC].pool);
	depthCommandBuf[1] = allocateCmdBuf(device, _renderHandle->queues[QueueType::GRAPHIC].pool);
	depthFence[0] = createFence(device, true);
	depthFence[1] = createFence(device, true);

	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = nullptr;
	semaphoreCreateInfo.flags = 0;

	vkCreateSemaphore(_renderHandle->getDevice(), &semaphoreCreateInfo, nullptr, &colorPassCompleteSemaphore);
}

void ShadowScene::transfer()
{

	lightInfoBuffer->transferData(&lightInfo, sizeof(lightInfo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	transformMatrixBuffer->transferData(&transformMatrix, sizeof(glm::mat4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
}

void ShadowScene::frame(float dt)
{
	switch (frameType)
	{
	case STANDARD:
		frame_standard(dt);
		break;
	case SINGLE_COMMAND_BUFFER:
		frame_single_cmdbuf(dt);
		break;
	case ASYNC:
		frame_async(dt);
		break;
	}
}

void ShadowScene::frame_standard(float dt)
{
	static float counter = 0.0f;
	counter += dt;
	createCameraMatrix(counter);

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
	VkRect2D scissor;

	vkCmdBeginRenderPass(info._buf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(info._buf, VK_PIPELINE_BIND_POINT_GRAPHICS, depthPassTechnique->pipeline);
	vkCmdSetViewport(info._buf, 0, 1, &shadowMapViewport);
	scissor.offset = { 0, 0 };
	scissor.extent = { shadowMapSize, shadowMapSize };
	vkCmdSetScissor(info._buf, 0, 1, &scissor);
	shadowMappingMatrixBuffer->bind(info._buf, _renderHandle->getFramePassLayout());
	//vkCmdBindDescriptorSets(info._buf, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeLayout._layout, 0, 1, &shadowPassDescriptorSet, 0, nullptr);

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
	scissor.extent = { _renderHandle->getWidth(), _renderHandle->getHeight() };
	vkCmdSetScissor(info._buf, 0, 1, &scissor);

	//Bind stuff
	lightInfoBuffer->bind(info._buf, _renderHandle->getFramePassLayout(), VK_PIPELINE_BIND_POINT_GRAPHICS);
	shadowMap->bind(info._buf, 1, _renderHandle->getFramePassLayout());
	transformMatrixBuffer->bind(info._buf, _renderHandle->getFramePassLayout());
	//vkCmdBindDescriptorSets(info._buf, VK_PIPELINE_BIND_POINT_GRAPHICS, _renderHandle->getFramePassLayout(), 1, 1, &renderPassDescriptorSet, 0, nullptr);

	vkCmdDraw(info._buf, positionBufferBinding.numElements, 1, 0, 0);

	_renderHandle->endRenderPass();
	// Submit
	_renderHandle->submitFramePass();

	post_standard();

	_renderHandle->present();
}

void ShadowScene::frame_single_cmdbuf(float dt)
{
	static float counter = 0.0f;
	counter += dt;
	createCameraMatrix(counter);

	VulkanRenderer::FrameInfo info = _renderHandle->beginGraphicsAndComputeCommandBuffer();

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
	VkRect2D scissor;

	vkCmdBeginRenderPass(info._buf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(info._buf, VK_PIPELINE_BIND_POINT_GRAPHICS, depthPassTechnique->pipeline);
	vkCmdSetViewport(info._buf, 0, 1, &shadowMapViewport);
	scissor.offset = { 0, 0 };
	scissor.extent = { shadowMapSize, shadowMapSize };
	vkCmdSetScissor(info._buf, 0, 1, &scissor);
	shadowMappingMatrixBuffer->bind(info._buf, _renderHandle->getFramePassLayout());
	//vkCmdBindDescriptorSets(info._buf, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeLayout._layout, 0, 1, &shadowPassDescriptorSet, 0, nullptr);

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
	scissor.extent = { _renderHandle->getWidth(), _renderHandle->getHeight() };
	vkCmdSetScissor(info._buf, 0, 1, &scissor);

	//Bind stuff
	lightInfoBuffer->bind(info._buf, _renderHandle->getFramePassLayout(), VK_PIPELINE_BIND_POINT_GRAPHICS);
	shadowMap->bind(info._buf, 1, _renderHandle->getFramePassLayout());
	transformMatrixBuffer->bind(info._buf, _renderHandle->getFramePassLayout());
	//vkCmdBindDescriptorSets(info._buf, VK_PIPELINE_BIND_POINT_GRAPHICS, _renderHandle->getFramePassLayout(), 1, 1, &renderPassDescriptorSet, 0, nullptr);

	vkCmdDraw(info._buf, positionBufferBinding.numElements, 1, 0, 0);

	_renderHandle->endGraphicsAndComputeRenderPass();
	// Submit

	transition_DepthWrite(info._buf, shadowMap->_imageHandle);
	transition_RenderToPost(info._buf, info._swapChainImage, _renderHandle->getQueueFamily(QueueType::GRAPHIC), _renderHandle->getQueueFamily(QueueType::GRAPHIC));

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
	transition_PostToPresent(info._buf, info._swapChainImage, _renderHandle->getQueueFamily(QueueType::GRAPHIC), _renderHandle->getQueueFamily(QueueType::GRAPHIC));
	_renderHandle->submitGraphicsAndCompute();

	_renderHandle->present();
}

void ShadowScene::frame_async(float dt)
{
	VulkanRenderer::FrameInfo info = _renderHandle->beginCommandBuffer();
	
	if (!firstFrame)
	{
		// Image barrier transferring image layout
		transition_DepthRead(info._buf, shadowMap->_imageHandle);

		// Rendering pass
		_renderHandle->beginRenderPass(info._buf);
		vkCmdBindPipeline(info._buf, VK_PIPELINE_BIND_POINT_GRAPHICS, renderPassTechnique->pipeline);
		VkViewport normalViewport = _renderHandle->getViewport();
		vkCmdSetViewport(info._buf, 0, 1, &normalViewport);
		VkRect2D scissor;
		scissor.offset = { 0, 0 };
		scissor.extent = { _renderHandle->getWidth(), _renderHandle->getHeight() };
		vkCmdSetScissor(info._buf, 0, 1, &scissor);

		//Bind stuff
		lightInfoBuffer->bind(info._buf, _renderHandle->getFramePassLayout(), VK_PIPELINE_BIND_POINT_GRAPHICS);
		shadowMap->bind(info._buf, 1, _renderHandle->getFramePassLayout());
		transformMatrixBuffer->bind(info._buf, _renderHandle->getFramePassLayout());
		//vkCmdBindDescriptorSets(info._buf, VK_PIPELINE_BIND_POINT_GRAPHICS, _renderHandle->getFramePassLayout(), 1, 1, &renderPassDescriptorSet, 0, nullptr);

		positionBufferBinding.bind(info._buf, 0);
		normalBufferBinding.bind(info._buf, 1);
		vkCmdDraw(info._buf, positionBufferBinding.numElements, 1, 0, 0);

		_renderHandle->endRenderPass();
		// Image barrier transferring image layout
		transition_DepthWrite(info._buf, shadowMap->_imageHandle);

	}
	// Depth pass
	static float counter = 0.0f;
	counter += dt;
	createCameraMatrix(counter);

	// Submit
	if (!firstFrame)
		_renderHandle->submitFramePass(colorPassCompleteSemaphore);
	else
		_renderHandle->submitFramePass();

	// Post
	if (!firstFrame)
		async_post(dt);
	async_depthBuffer(dt);

	_renderHandle->present(firstFrame);

	firstFrame = false;
}

void ShadowScene::async_post(float dt)
{
	// Post
	VulkanRenderer::FrameInfo info = _renderHandle->beginCompute(0);
	transition_RenderToPost(info._buf, info._swapChainImage, _renderHandle->getQueueFamily(QueueType::GRAPHIC), _renderHandle->getQueueFamily(QueueType::COMPUTE));

	// Bind compute shader
	techniqueBlurHorizontal->bind(info._buf, VK_PIPELINE_BIND_POINT_COMPUTE);
	vkCmdBindDescriptorSets(info._buf, VK_PIPELINE_BIND_POINT_COMPUTE, postLayout._layout, 0, 1, &swapChainImgDesc[info._swapChainIndex], 0, nullptr);
	// Dispatch
	vkCmdDispatch(info._buf, 1, _renderHandle->getHeight(), 1);

	serializeCommandBuffer(info._buf);
	// Bind compute shader
	techniqueBlurVertical->bind(info._buf, VK_PIPELINE_BIND_POINT_COMPUTE);
	// Dispatch
	vkCmdDispatch(info._buf, 1, _renderHandle->getWidth(), 1);

	// Finish
	transition_PostToPresent(info._buf, info._swapChainImage, _renderHandle->getQueueFamily(QueueType::COMPUTE), _renderHandle->getQueueFamily(QueueType::GRAPHIC));
	_renderHandle->submitCompute(0, true, colorPassCompleteSemaphore);
}

void ShadowScene::async_depthBuffer(float dt)
{
	VkCommandBuffer cmdBuf = depthCommandBuf[_renderHandle->getFrameIndex()];
	waitFence(_renderHandle->getDevice(), depthFence[_renderHandle->getFrameIndex()]);
	VkResult err = vkResetCommandBuffer(cmdBuf, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
	if (err)
		std::cout << "Command buff reset err\n";
	// Begin recording frame commands
	beginCmdBuf(cmdBuf, VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT | VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
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

	vkCmdBeginRenderPass(cmdBuf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, depthPassTechnique->pipeline);
	vkCmdSetViewport(cmdBuf, 0, 1, &shadowMapViewport);
	VkRect2D scissor;
	scissor.offset = { 0, 0 };
	scissor.extent = { shadowMapSize, shadowMapSize };
	vkCmdSetScissor(cmdBuf, 0, 1, &scissor);
	shadowMappingMatrixBuffer->bind(cmdBuf, _renderHandle->getFramePassLayout());
	//vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeLayout._layout, 0, 1, &shadowPassDescriptorSet, 0, nullptr);

	positionBufferBinding.bind(cmdBuf, 0);
	normalBufferBinding.bind(cmdBuf, 1);
	vkCmdDraw(cmdBuf, positionBufferBinding.numElements, 1, 0, 0);
	vkCmdEndRenderPass(cmdBuf);

	if (vkEndCommandBuffer(cmdBuf) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT };
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = NULL;
	submitInfo.pWaitDstStageMask = waitStages;	// Mask stage where related semaphore is waited for.
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuf;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = NULL;
	err = vkQueueSubmit(_renderHandle->queues[QueueType::GRAPHIC].queue, 1, &submitInfo, depthFence[_renderHandle->getFrameIndex()]);
	if (err != VK_SUCCESS) {
		throw std::runtime_error("Failed to submit draw command buffer!");
	}
}


void ShadowScene::post_standard()
{
	VulkanRenderer::FrameInfo info = _renderHandle->beginCompute();
	transition_DepthWrite(info._buf, shadowMap->_imageHandle);
	transition_RenderToPost(info._buf, info._swapChainImage, _renderHandle->getQueueFamily(QueueType::GRAPHIC), _renderHandle->getQueueFamily(QueueType::COMPUTE));

	// Bind compute shader
	techniqueBlurHorizontal->bind(info._buf, VK_PIPELINE_BIND_POINT_COMPUTE);
	vkCmdBindDescriptorSets(info._buf, VK_PIPELINE_BIND_POINT_COMPUTE, postLayout._layout, 0, 1, &swapChainImgDesc[info._swapChainIndex], 0, nullptr);
	// Dispatch
	vkCmdDispatch(info._buf, 1, _renderHandle->getHeight(), 1);


	// Bind compute shader
	techniqueBlurVertical->bind(info._buf, VK_PIPELINE_BIND_POINT_COMPUTE);
	// Dispatch
	vkCmdDispatch(info._buf, 1, _renderHandle->getWidth(), 1);

	// Finish
	transition_PostToPresent(info._buf, info._swapChainImage, _renderHandle->getQueueFamily(QueueType::COMPUTE), _renderHandle->getQueueFamily(QueueType::GRAPHIC));
	_renderHandle->submitCompute(0, true);
}


void ShadowScene::defineDescriptorLayout(VkDevice device, std::vector<VkDescriptorSetLayout>& layout)
{
	layout.resize(3);
	
	// Shadow map
	VkDescriptorSetLayoutBinding binding;

	// transformMatrix
	writeLayoutBinding(binding, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	layout[0] = createDescriptorLayout(device, &binding, 1);

	writeLayoutBinding(binding, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	layout[1] = createDescriptorLayout(device, &binding, 1);
	
	// clipSpaceToShadowMapMatrix
	writeLayoutBinding(binding, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
	layout[2] = createDescriptorLayout(device, &binding, 1);
}


VkRenderPass ShadowScene::defineRenderPass(VkDevice device, VkFormat swapchainFormat, VkFormat depthFormat, std::vector<VkImageView>& additionalAttatchments)
{
	return createRenderPass_SingleColorDepth(device, swapchainFormat, depthFormat);
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
	// Shadow pass layout
	shadowPipeLayout = vk::LayoutConstruct(1);
	VkDescriptorSetLayoutBinding binding;
	writeLayoutBinding(binding, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	shadowPipeLayout[0] = createDescriptorLayout(_renderHandle->getDevice(), &binding, 1);
	shadowPipeLayout.construct(_renderHandle->getDevice());

	// Create image and sampler
	// Shouldn't really be done here, but the renderer needs the shadow map image view at this point
	shadowMapSampler = new Sampler2DVulkan(_renderHandle);
	shadowMapSampler->setMinFilter(VkFilter::VK_FILTER_LINEAR);
	shadowMapSampler->setMagFilter(VkFilter::VK_FILTER_LINEAR);
	//shadowMapSampler->setWrap(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);

	shadowMap = new Texture2DVulkan(_renderHandle, shadowMapSampler);
	shadowMap->createShadowMap(shadowMapSize, shadowMapSize, shadowMapFormat);

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

	const uint32_t DEPENDENCY_COUNT = 1;
	VkSubpassDependency dependencies[DEPENDENCY_COUNT] = { {} };
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependencies[0].srcAccessMask = 0;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	//dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	//dependencies[0].dstSubpass = 0;
	//dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	//dependencies[0].dstStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	//dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	//dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	//dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	//dependencies[1].srcSubpass = 0;
	//dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	//dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	//dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	//dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	//dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	//dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.pNext = nullptr;
	renderPassCreateInfo.flags = 0;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &attatchment;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = DEPENDENCY_COUNT;
	renderPassCreateInfo.pDependencies = dependencies;


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

	transformMatrixBuffer = new ConstantDoubleBufferVulkan(_renderHandle);
	transformMatrixBuffer->setData(&transformMatrix, sizeof(glm::mat4), 0, _renderHandle->getDescriptorSetLayout(0), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

	lightInfoBuffer = new ConstantDoubleBufferVulkan(_renderHandle);
	lightInfoBuffer->setData(&lightInfo, sizeof(lightInfo), 2, _renderHandle->getDescriptorSetLayout(2));
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
	glm::mat4 tra = glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f * sinf(time), 0.5f * sinf(time * 0.1f), -30.0f));
	glm::mat4 per = perspectiveMatrix(static_cast<float>(_renderHandle->getWidth()) / static_cast<float>(_renderHandle->getHeight()), 
		1.0f, 0.1f, 60.0f);
	//glm::mat4 per = orthographicMatrix(-4.0f, 4.0f, -4.0f, 4.0f, 0.1f, 10.0f) * cameraMatrix;

	transformMatrix = per * tra * rot * cameraMatrix;
	lightInfo.lightDirection = glm::inverse(rot) * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
}
