#include "Scenes/ComputeScene.h"
#include "VulkanRenderer.h"
#include "Stuff/RandomGenerator.h"
#include "VulkanConstruct.h"

ComputeScene::ComputeScene(Mode mode)
	: mode(mode)
{
}


ComputeScene::~ComputeScene()
{
	delete techniqueA;
	delete triShader;
	delete triBuffer;

	delete techniquePost, delete techniqueBlurHorizontal, delete techniqueBlurVertical;
	delete compShader, delete blurHorizontal, delete blurVertical;
	delete readImg;
	delete readSampler;
	postLayout.destroy(_renderHandle->getDevice());
}

void ComputeScene::initialize(VulkanRenderer *handle)
{
	Scene::initialize(handle);

	// Render pass initiation
	triShader = new ShaderVulkan("testShaders", _renderHandle);
#ifdef COMPILE
	triShader->setShader("resource/trishader/TriVertex.glsl", ShaderVulkan::ShaderType::VS);
	triShader->setShader("resource/trishader/TriFragment.glsl", ShaderVulkan::ShaderType::PS);
#else
	triShader->setShader("resource/tmp/TriVertex.spv", ShaderVulkan::ShaderType::VS);
	triShader->setShader("resource/tmp/TriFragment.spv", ShaderVulkan::ShaderType::PS);
#endif
	std::string err;
	triShader->compileMaterial(err);


	// Create testing vertex buffer
	const uint32_t NUM_TRIS = 10000;
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
	postLayout = vk::LayoutConstruct(2);
	writeLayoutBinding(binding, 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
	postLayout[0] = createDescriptorLayout(_renderHandle->getDevice(), &binding, 1);
	writeLayoutBinding(binding, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT);
	postLayout[1] = createDescriptorLayout(_renderHandle->getDevice(), &binding, 1);
	// Gen. layout
	postLayout.construct(_renderHandle->getDevice());


	compShader = new ShaderVulkan("CopyCompute", _renderHandle);
	compShader->setShader("resource/Compute/CopyTexture.glsl", ShaderVulkan::ShaderType::CS);
	compShader->compileMaterial(err);

	blurHorizontal = new ShaderVulkan("CopyCompute", _renderHandle);
	blurHorizontal->setShader("resource/Compute/GaussianHorizontal.glsl", ShaderVulkan::ShaderType::CS);
	blurHorizontal->compileMaterial(err);

	blurVertical = new ShaderVulkan("CopyCompute", _renderHandle);
	blurVertical->setShader("resource/Compute/GaussianVertical.glsl", ShaderVulkan::ShaderType::CS);
	blurVertical->compileMaterial(err);
	
	// Img source
	readSampler = new Sampler2DVulkan(_renderHandle);
	readSampler->setWrap(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	readSampler->setMagFilter(VkFilter::VK_FILTER_NEAREST);
	readSampler->setMinFilter(VkFilter::VK_FILTER_NEAREST);
	readImg = new Texture2DVulkan(_renderHandle, readSampler);
	readImg->loadFromFile("resource/fatboy.png");
	readImg->attachBindPoint(1, postLayout[1]);

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

	makeTechnique();
}

void ComputeScene::makeTechnique()
{

	techniquePost = new TechniqueVulkan(_renderHandle, compShader, postLayout._layout);
	techniqueBlurHorizontal = new TechniqueVulkan(_renderHandle, blurHorizontal, postLayout._layout);
	techniqueBlurVertical = new TechniqueVulkan(_renderHandle, blurVertical, postLayout._layout);

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
	techniqueA = new TechniqueVulkan(_renderHandle, triShader, _renderHandle->getFramePass(), _renderHandle->getFramePassLayout(), vertexBindings);
}


void ComputeScene::mainPass()
{
	// Main render pass
	VulkanRenderer::FrameInfo info = _renderHandle->beginFramePass();
	vkCmdSetViewport(info._buf, 0, 1, &_renderHandle->getViewport());
	VkRect2D scissor;
	scissor.offset = { 0, 0 };
	scissor.extent = { _renderHandle->getWidth(), _renderHandle->getHeight() };
	vkCmdSetScissor(info._buf, 0, 1, &scissor);

	vkCmdBindPipeline(info._buf, VK_PIPELINE_BIND_POINT_GRAPHICS, techniqueA->pipeline);

	VkDeviceSize offsets = 0;
	triVertexBinding.bind(info._buf, 0);
	vkCmdDraw(info._buf, (uint32_t)triVertexBinding.numElements, 1, 0, 0);

	// Subpass...
	//vkCmdNextSubpass(cmdBuf, VK_SUBPASS_CONTENTS_INLINE); // Subpass is inlined in primary command buffer

	// Finish
	_renderHandle->endRenderPass();
	_renderHandle->submitFramePass();

}
void ComputeScene::post(VulkanRenderer::FrameInfo info)
{
	// Post pass
	transition_RenderToPost(info._buf, info._swapChainImage, _renderHandle->getQueueFamily(QueueType::GRAPHIC), _renderHandle->getQueueFamily(QueueType::COMPUTE));

	// Bind compute shader
	techniquePost->bind(info._buf, VK_PIPELINE_BIND_POINT_COMPUTE);
	// Bind resources
 	readImg->bind(info._buf, 1, postLayout._layout, VK_PIPELINE_BIND_POINT_COMPUTE);
	vkCmdBindDescriptorSets(info._buf, VK_PIPELINE_BIND_POINT_COMPUTE, postLayout._layout, 0, 1, &swapChainImgDesc[info._swapChainIndex], 0, nullptr);
	// Dispatch
	uint32_t dim = 512 / 16;
	vkCmdDispatch(info._buf, dim, dim, 1);
}

void ComputeScene::postBlur(VulkanRenderer::FrameInfo info)
{

	// Bind compute shader
	techniqueBlurHorizontal->bind(info._buf, VK_PIPELINE_BIND_POINT_COMPUTE);
	vkCmdBindDescriptorSets(info._buf, VK_PIPELINE_BIND_POINT_COMPUTE, postLayout._layout, 0, 1, &swapChainImgDesc[info._swapChainIndex], 0, nullptr);
	// Dispatch
	vkCmdDispatch(info._buf, 1, _renderHandle->getHeight(), 1);


	// Bind compute shader
	techniqueBlurVertical->bind(info._buf, VK_PIPELINE_BIND_POINT_COMPUTE);
	// Dispatch
	vkCmdDispatch(info._buf, 1, _renderHandle->getWidth(), 1);
}

void ComputeScene::transfer()
{

}

void ComputeScene::frame(float dt)
{
	mainPass();
	VulkanRenderer::FrameInfo info = _renderHandle->beginCompute();

	post(info);
	if(mode == Mode::Blur)
		postBlur(info);

	// Finish
	transition_PostToPresent(info._buf, info._swapChainImage, _renderHandle->getQueueFamily(QueueType::COMPUTE), _renderHandle->getQueueFamily(QueueType::GRAPHIC));
	_renderHandle->submitCompute();
	_renderHandle->present();
}



void ComputeScene::defineDescriptorLayout(VkDevice device, std::vector<VkDescriptorSetLayout> &layout)
{
	layout.resize(0);
}


VkRenderPass ComputeScene::defineRenderPass(VkDevice device, VkFormat swapchainFormat, VkFormat depthFormat, std::vector<VkImageView>& additionalAttatchments)
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