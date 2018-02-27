#include "Scenes/ComputeScene.h"
#include "VulkanRenderer.h"
#include "Stuff/RandomGenerator.h"
#include "VulkanConstruct.h"

ComputeScene::ComputeScene()
{
}


ComputeScene::~ComputeScene()
{
	delete techniqueA;
	delete triShader;
	delete triBuffer;

	delete techniquePost;
	delete computeShader;
	delete readImg;
	delete readSampler;
}

void ComputeScene::initialize(VulkanRenderer *handle)
{
	Scene::initialize(handle);

	// Render pass initiation
	triShader = new ShaderVulkan("testShaders", _renderHandle);
	triShader->setShader("resource/trishader/VertexShader.glsl", ShaderVulkan::ShaderType::VS);
	triShader->setShader("resource/trishader/FragmentShader.glsl", ShaderVulkan::ShaderType::PS);
	std::string err;
	triShader->compileMaterial(err);


	// Create testing vertex buffer
	const uint32_t NUM_TRIS = 10;
	mf::RandomGenerator rnd;
	rnd.seedGenerator();
	glm::vec4 testTriangles[NUM_TRIS * 3];
	mf::distributeTriangles(rnd, 1.f, NUM_TRIS, glm::vec2(0.1f, 0.5f), testTriangles, nullptr);

	triBuffer = new VertexBufferVulkan(_renderHandle, sizeof(glm::vec4) * NUM_TRIS * 3, VertexBufferVulkan::DATA_USAGE::STATIC);
	triVertexBinding = VertexBufferVulkan::Binding(triBuffer, sizeof(glm::vec4), NUM_TRIS * 3, 0);
	triBuffer->setData(testTriangles, triVertexBinding.byteSize(), 0);

	// Post pass initiation
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

	makeTechnique();
}

void ComputeScene::makeTechnique()
{
	techniquePost = new TechniqueVulkan(computeShader, _renderHandle);


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
	techniqueA = new TechniqueVulkan(triShader, _renderHandle, _renderHandle->getFramePass(), vertexBindings);
}

void ComputeScene::frame(VkCommandBuffer cmdBuf)
{

	// Main render pass
	_renderHandle->beginFramePass();

	vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, techniqueA->pipeline);

	VkDeviceSize offsets = 0;
	triVertexBinding.bind(0);
	vkCmdDraw(cmdBuf, (uint32_t)triVertexBinding.numElements, 1, 0, 0);

	// Post pass
	vkCmdNextSubpass(cmdBuf, VK_SUBPASS_CONTENTS_INLINE); // Subpass is inlined in primary command buffer
	techniqueA->bind(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS);
	readImg->bind(cmdBuf, 0);

	//vkCmdDispatch(commandBuffer, bufferSize / sizeof(int32_t), 1, 1);
	//...
	_renderHandle->submitFramePass();

	_renderHandle->present(true, false);
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


VkRenderPass ComputeScene::defineRenderPass(VkDevice device, VkFormat swapchainFormat, VkFormat depthFormat)
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
	subpass[1].flags = 0;
	subpass[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // Must be bind point GRAPHICS... VK_PIPELINE_BIND_POINT_COMPUTE not 'supported'
	subpass[1].inputAttachmentCount = 0;
	subpass[1].pInputAttachments = NULL;
	subpass[1].colorAttachmentCount = 0;
	subpass[1].pColorAttachments = NULL;
	subpass[1].pResolveAttachments = NULL;
	subpass[1].pDepthStencilAttachment = NULL;
	subpass[1].preserveAttachmentCount = 0;

	// Specify dependency for transitioning the image layout for rendering. 
	const uint32_t NUM_DEPENDENCY = 2;
	VkSubpassDependency dependency[NUM_DEPENDENCY];
	dependency[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency[0].dstSubpass = 0;
	dependency[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency[0].srcAccessMask = 0;
	dependency[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency[0].dependencyFlags = 0;

	// Transition post pass
	dependency[1].srcSubpass = 0;
	dependency[1].dstSubpass = 1;
	dependency[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency[1].srcAccessMask = 0;
	dependency[1].dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	dependency[1].dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	dependency[1].dependencyFlags = 0;

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