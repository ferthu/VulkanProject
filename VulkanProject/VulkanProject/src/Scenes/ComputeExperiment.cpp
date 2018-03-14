#include "Scenes/ComputeExperiment.h"
#include "VulkanRenderer.h"
#include "Stuff/RandomGenerator.h"
#include "VulkanConstruct.h"
#include <iostream>

ComputeExperiment::ComputeExperiment(Mode mode, ShaderMode shader, uint32_t num_particles)
	: mode(mode), shaderMode(shader), NUM_PARTICLE(num_particles)
{
}


ComputeExperiment::~ComputeExperiment()
{
	delete techniqueA;
	delete triShader;
	delete triBuffer;

	delete techniquePost, delete techniqueSmallOp;
	delete compShader, delete compSmallOp;
	delete smallOpBuf;
	smallOpLayout.destroy(_renderHandle->getDevice());
	postLayout.destroy(_renderHandle->getDevice());

	if (shaderMode == ShaderMode::MEM_LIMITED)
	{
		delete readImg;
		delete readSampler;
	}
}
//#define COMPILE

void ComputeExperiment::initialize(VulkanRenderer *handle)
{
	std::cout << "Initiating Compute Experiment\n";
	std::cout << "Mode: " << mode << "\n";
	Scene::initialize(handle);

	// Render pass initiation
	std::string err;
	triShader = new ShaderVulkan("testShaders", _renderHandle);
#ifdef COMPILE
	triShader->setShader("resource/trishader/VertexShader.glsl", ShaderVulkan::ShaderType::VS);
	triShader->setShader("resource/trishader/FragmentShader.glsl", ShaderVulkan::ShaderType::PS);
#else
	triShader->setShader("resource/tmp/VertexShader.spv", ShaderVulkan::ShaderType::VS);
	triShader->setShader("resource/tmp/FragmentShader.spv", ShaderVulkan::ShaderType::PS);
#endif
	triShader->compileMaterial(err);


	// Create testing vertex buffer
	const uint32_t NUM_TRIS = 100;
	mf::RandomGenerator rnd;
	rnd.seedGenerator();
	glm::vec4 testTriangles[NUM_TRIS * 3];
	mf::distributeTriangles(rnd, 1.f, NUM_TRIS, glm::vec2(0.1f, 0.5f), testTriangles, nullptr);

	triBuffer = new VertexBufferVulkan(_renderHandle, sizeof(glm::vec4) * NUM_TRIS * 3, VertexBufferVulkan::DATA_USAGE::STATIC);
	triVertexBinding = VertexBufferVulkan::Binding(triBuffer, sizeof(glm::vec4), NUM_TRIS * 3, 0);
	triBuffer->setData(testTriangles, triVertexBinding.byteSize(), 0);

	// Post pass initiation

	//Layout
	VkDescriptorSetLayoutBinding binding;
	postLayout = vk::LayoutConstruct(shaderMode == ShaderMode::MEM_LIMITED ? 2 : 1);
	writeLayoutBinding(binding, 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
	postLayout[0] = createDescriptorLayout(_renderHandle->getDevice(), &binding, 1);
	if (shaderMode == ShaderMode::MEM_LIMITED)
	{
		writeLayoutBinding(binding, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT);
		postLayout[1] = createDescriptorLayout(_renderHandle->getDevice(), &binding, 1);
	}
	// Gen. layout
	postLayout.construct(_renderHandle->getDevice());


	compShader = new ShaderVulkan("CopyCompute", _renderHandle);
	compSmallOp = new ShaderVulkan("SmallOp", _renderHandle);
#ifdef COMPILE
	compSmallOp->setShader("resource/Compute/ComputeSimple.glsl", ShaderVulkan::ShaderType::CS);
	if (shaderMode == ShaderMode::MEM_LIMITED)
		compShader->setShader("resource/Compute/ComputeMemLimited.glsl", ShaderVulkan::ShaderType::CS);
	else
		compShader->setShader("resource/Compute/ComputeRegLimited.glsl", ShaderVulkan::ShaderType::CS);
#else
	compSmallOp->setShader("resource/tmp/ComputeSimple.spv", ShaderVulkan::ShaderType::CS);
	if (shaderMode == ShaderMode::MEM_LIMITED)
		compShader->setShader("resource/tmp/ComputeMemLimited.spv", ShaderVulkan::ShaderType::CS);
	else
		compShader->setShader("resource/tmp/ComputeRegLimited.spv", ShaderVulkan::ShaderType::CS);
#endif
	compShader->compileMaterial(err);
	compSmallOp->compileMaterial(err);

	// Image
	if (shaderMode == ShaderMode::MEM_LIMITED)
	{
		readSampler = new Sampler2DVulkan(_renderHandle);
		readSampler->setWrap(VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT);
		readSampler->setMagFilter(VkFilter::VK_FILTER_NEAREST);
		readSampler->setMinFilter(VkFilter::VK_FILTER_NEAREST);
		readImg = new Texture2DVulkan(_renderHandle, readSampler);
		readImg->loadFromFile("resource/fatboy.png");
		readImg->attachBindPoint(1, postLayout[1]);
	}
	// Framebuf targets
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



	makeTechnique();
}

void ComputeExperiment::makeTechnique()
{
	techniquePost = new TechniqueVulkan(_renderHandle, compShader, postLayout._layout);
	// Gen. particle layout
	VkDescriptorSetLayoutBinding binding;
	smallOpLayout = vk::LayoutConstruct(1);
	writeLayoutBinding(binding, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
	smallOpLayout[0] = createDescriptorLayout(_renderHandle->getDevice(), &binding, 1);
	smallOpLayout.construct(_renderHandle->getDevice());
	// Gen. particle buffer
	struct Particle
	{
		float x; glm::vec2 pos, vel;
	};
	std::unique_ptr<Particle> arr(new Particle[NUM_PARTICLE]);
	for (size_t i = 0; i < NUM_PARTICLE; i++)
		arr.get()[i] = { 0, glm::vec2(cos(i), sin(i)), glm::vec2(-cos(i), -sin(i)) };
	smallOpBuf = new ConstantBufferVulkan(_renderHandle);
	smallOpBuf->setData(arr.get(), sizeof(Particle) * NUM_PARTICLE, 0, smallOpLayout[0], VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	// Gen. technique
	techniqueSmallOp = new TechniqueVulkan(_renderHandle, compSmallOp, smallOpLayout._layout);


	const uint32_t NUM_BUFFER = 1;
	const uint32_t NUM_ATTRI = 1;
	VkVertexInputBindingDescription vertexBufferBindings[NUM_BUFFER] =
	{
		defineVertexBinding(0, 16),
		/*defineVertexBinding(NORMAL, 16),
		defineVertexBinding(TEXTCOORD, 8)*/
	};
	VkVertexInputAttributeDescription vertexAttributes[NUM_ATTRI] =
	{
		defineVertexAttribute(0, 0, VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT, 0),
		/*defineVertexAttribute(NORMAL, NORMAL, VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT, 0),
		defineVertexAttribute(TEXTCOORD, TEXTCOORD, VkFormat::VK_FORMAT_R32G32_SFLOAT, 0)*/
	};
	VkPipelineVertexInputStateCreateInfo vertexBindings =
		defineVertexBufferBindings(vertexBufferBindings, NUM_BUFFER, vertexAttributes, NUM_ATTRI);
	//techniqueA = new TechniqueVulkan(_renderHandle, triShader, _renderHandle->getFramePass(), _renderHandle->getFramePassLayout(), vertexBindings);
}

void ComputeExperiment::frame()
{
	// Main render pass
	VulkanRenderer::FrameInfo info = _renderHandle->beginFramePass();
	vkCmdSetViewport(info._buf, 0, 1, &_renderHandle->getViewport());
	/*
	vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, techniqueA->pipeline);

	VkDeviceSize offsets = 0;
	triVertexBinding.bind(0);
	vkCmdDraw(cmdBuf, (uint32_t)triVertexBinding.numElements, 1, 0, 0);
	// Subpass...
	//vkCmdNextSubpass(cmdBuf, VK_SUBPASS_CONTENTS_INLINE); // Subpass is inlined in primary command buffer

	*/
	// Finish
	_renderHandle->submitFramePass();


	// Post pass
	info = _renderHandle->beginCompute();
	transition_ComputeToPost(info._buf, info._swapChainImage, _renderHandle->getQueueFamily(QueueType::GRAPHIC), _renderHandle->getQueueFamily(QueueType::COMPUTE));

	// Dispatch frame compute shader
	techniquePost->bind(info._buf, VK_PIPELINE_BIND_POINT_COMPUTE);
	// Bind resources
	vkCmdBindDescriptorSets(info._buf, VK_PIPELINE_BIND_POINT_COMPUTE, postLayout._layout, 0, 1, &swapChainImgDesc[info._swapChainIndex], 0, nullptr);
	if(shaderMode == ShaderMode::MEM_LIMITED)
		readImg->bind(info._buf, 1, postLayout._layout, VK_PIPELINE_BIND_POINT_COMPUTE);
	// Dispatch
	uint32_t dimX = _renderHandle->getWidth() / 16;
	uint32_t dimY = _renderHandle->getHeight() / 16;
	if (mode == Mode::MULTI_DISPATCH)
	{
		for (int y = 0; y < _renderHandle->getHeight() / (16 * 8); y++)
		{
			for (int x = 0; x < _renderHandle->getWidth() / (16 * 8); x++)
				vkCmdDispatch(info._buf, 8, 8, 1);
		}
	}
	else
		vkCmdDispatch(info._buf, dimX, dimY, 1);

	if (mode == Mode::MULTI_QUEUE)
	{
		transition_PostToPresent(info._buf, info._swapChainImage, _renderHandle->getQueueFamily(QueueType::COMPUTE), _renderHandle->getQueueFamily(QueueType::GRAPHIC));
		_renderHandle->submitCompute(0, true);
		info = _renderHandle->beginCompute(1);
	}
	else if (mode == Mode::SEQUENTIAL)
		serializeCommandBuffer(info._buf);
	
	// Dispatch compute operation
	techniqueSmallOp->bind(info._buf, VK_PIPELINE_BIND_POINT_COMPUTE);
	smallOpBuf->bind(info._buf, smallOpLayout._layout, VK_PIPELINE_BIND_POINT_COMPUTE);

	if (mode == Mode::MULTI_DISPATCH)
	{
		for (int i = 0; i < NUM_PARTICLE / (256 * 64); i++)
			vkCmdDispatch(info._buf, 64, 1, 1);
	}
	else
		vkCmdDispatch(info._buf, NUM_PARTICLE / 256, 1, 1);

	//Transition frame buf back
	if (mode == Mode::MULTI_QUEUE)
	{
		_renderHandle->submitCompute(1, false);
	}
	else
	{
		transition_PostToPresent(info._buf, info._swapChainImage, _renderHandle->getQueueFamily(QueueType::COMPUTE), _renderHandle->getQueueFamily(QueueType::GRAPHIC));
		_renderHandle->submitCompute(0, true);
	}
	
	_renderHandle->present();
}


void ComputeExperiment::defineDescriptorLayout(VkDevice device, std::vector<VkDescriptorSetLayout> &layout)
{
	layout.resize(0);
}


VkRenderPass ComputeExperiment::defineRenderPass(VkDevice device, VkFormat swapchainFormat, VkFormat depthFormat, std::vector<VkImageView>& additionalAttatchments)
{
	const int num_attach = 2;
	VkAttachmentDescription attach[num_attach] = {
		defineFramebufColor(swapchainFormat),
		defineFramebufDepth(depthFormat)
	};
	// Referenced renderbuffer target
	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference postAttachmentRef = {};
	postAttachmentRef.attachment = 0;
	postAttachmentRef.layout = VK_IMAGE_LAYOUT_GENERAL;

	const uint32_t NUM_SUBPASS = 1;
	VkSubpassDescription subpass[NUM_SUBPASS];
	subpass[0].flags = 0;
	subpass[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass[0].inputAttachmentCount = 0;
	subpass[0].pInputAttachments = NULL;
	subpass[0].colorAttachmentCount = 1;
	subpass[0].pColorAttachments = &colorAttachmentRef;
	subpass[0].pResolveAttachments = NULL;
	subpass[0].pDepthStencilAttachment = &depthAttachmentRef;
	subpass[0].preserveAttachmentCount = 0;


	// Technically there would proably be some nifty way of preserving some state...
	/*
	subpass[1].flags = 0;
	subpass[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // Must be bind point GRAPHICS... VK_PIPELINE_BIND_POINT_COMPUTE not 'supported'
	subpass[1].inputAttachmentCount = 0;
	subpass[1].pInputAttachments = NULL;
	...
	*/

	// Specify dependency for transitioning the image layout for rendering. 
	const uint32_t NUM_DEPENDENCY = 1;
	VkSubpassDependency dependency[NUM_DEPENDENCY];
	dependency[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency[0].dstSubpass = 0;
	dependency[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency[0].srcAccessMask = 0;
	dependency[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency[0].dependencyFlags = 0;

	// Transition post pass
	/*
	dependency[1].srcSubpass = 0;
	dependency[1].dstSubpass = 1;
	dependency[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	...
	*/
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = num_attach;
	renderPassInfo.pAttachments = attach;
	renderPassInfo.subpassCount = NUM_SUBPASS;
	renderPassInfo.pSubpasses = subpass;
	renderPassInfo.dependencyCount = NUM_DEPENDENCY;
	renderPassInfo.pDependencies = dependency;

	VkRenderPass pass;
	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &pass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
	return pass;
}