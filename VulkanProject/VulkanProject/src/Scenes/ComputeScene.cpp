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