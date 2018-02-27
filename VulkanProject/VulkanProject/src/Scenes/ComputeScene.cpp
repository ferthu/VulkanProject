#include "Scenes/ComputeScene.h"
#include "VulkanRenderer.h"
#include "Stuff/RandomGenerator.h"


ComputeScene::ComputeScene()
{
}


ComputeScene::~ComputeScene()
{
	delete techniqueA;
	delete computeShader;
	delete readImg;
	delete readSampler;
}

void ComputeScene::initialize(VulkanRenderer *handle)
{
	Scene::initialize(handle);
	std::string err;
	computeShader = new ShaderVulkan("CopyCompute", _renderHandle);
	computeShader->setShader("resource/Compute/CopyTexture.glsl", ShaderVulkan::ShaderType::CS);
	computeShader->compileMaterial(err);

	// Img source
	readSampler = new Sampler2DVulkan(_renderHandle);
	readSampler->setWrap(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	readSampler->setMagFilter(VkFilter::VK_FILTER_NEAREST);
	readSampler->setMinFilter(VkFilter::VK_FILTER_NEAREST);
	readImg = new Texture2DVulkan(_renderHandle, readSampler);
	readImg->loadFromFile("resource/fatboy.png");
	techniqueA = new TechniqueVulkan(computeShader, _renderHandle);
}

void ComputeScene::makeTechniqueA()
{
	techniqueA = new TechniqueVulkan(computeShader, _renderHandle);
}

void ComputeScene::frame(VkCommandBuffer cmdBuf)
{
	_renderHandle->beginCompute();
	VkCommandBuffer compBuf = _renderHandle->getComputeBuf();


	techniqueA->bind(compBuf, VK_PIPELINE_BIND_POINT_COMPUTE);
	readImg->bind(compBuf, 0);

	_renderHandle->submitCompute();


	//vkCmdDispatch(commandBuffer, bufferSize / sizeof(int32_t), 1, 1);
	//...

	_renderHandle->beginFramePass();
	_renderHandle->submitFramePass();

	_renderHandle->present(true, true);
}


void ComputeScene::defineDescriptorLayout(VkDevice device, std::vector<VkDescriptorSetLayout> &layout)
{
	layout.resize(2);
	VkDescriptorSetLayoutBinding binding, binding2;
	writeLayoutBinding(binding, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT);
	writeLayoutBinding(binding2, 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
	layout[0] = createDescriptorLayout(device, &binding, 1);
	layout[1] = createDescriptorLayout(device, &binding2, 1);
}


VkRenderPass ComputeScene::defineRenderPass(VkFormat swapchainFormat, VkFormat depthFormat)
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

	const uint32_t NUM_SUBPASS = 2;
	VkSubpassDescription subpass[NUM_SUBPASS];
	subpass[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass[0].colorAttachmentCount = 1;
	subpass[0].pColorAttachments = &colorAttachmentRef;
	subpass[0].pDepthStencilAttachment = &depthAttachmentRef;

	// Technically there would proably be some nifty way of preserving some state...
	subpass[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
	subpass[1].colorAttachmentCount = 1;
	subpass[1].pColorAttachments = &colorAttachmentRef;
	subpass[1].pDepthStencilAttachment = &depthAttachmentRef;

	// Specify dependency for transitioning the image layout for rendering. 
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = num_attach;
	renderPassInfo.pAttachments = attach;
	renderPassInfo.subpassCount = NUM_SUBPASS;
	renderPassInfo.pSubpasses = subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	VkRenderPass pass;
	if (vkCreateRenderPass(_renderHandle->getDevice(), &renderPassInfo, nullptr, &pass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
	return pass;
}