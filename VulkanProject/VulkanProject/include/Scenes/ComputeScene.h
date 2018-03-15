#pragma once
#include "..\Scene.h"

#include "vulkan\vulkan.h"
#include "glm\glm.hpp"
#include "VertexBufferVulkan.h"
#include "ShaderVulkan.h"
#include "Texture2DVulkan.h"
#include "Sampler2DVulkan.h"


class ComputeScene :
	public Scene
{
public:

	enum Mode {
		Sequential,
		Blur
	};

	ComputeScene(Mode mode = Sequential);
	~ComputeScene();

	virtual void frame();
	virtual void transfer();
	virtual void initialize(VulkanRenderer *handle);
	virtual void defineDescriptorLayout(VkDevice device, std::vector<VkDescriptorSetLayout> &layout);
	virtual VkRenderPass defineRenderPass(VkDevice device, VkFormat swapchainFormat, VkFormat depthFormat, std::vector<VkImageView>& additionalAttatchments);

private:

	Mode mode;

	void makeTechnique();

	void mainPass();
	void post(VulkanRenderer::FrameInfo info);
	void postBlur(VulkanRenderer::FrameInfo info);

	// Render pass
	ShaderVulkan *triShader;
	TechniqueVulkan * techniqueA;
	VertexBufferVulkan* triBuffer;
	VertexBufferVulkan::Binding triVertexBinding;

	// Post pass
	TechniqueVulkan * techniquePost, *techniqueBlurHorizontal, *techniqueBlurVertical;
	vk::LayoutConstruct postLayout;
	ShaderVulkan *compShader, *blurHorizontal, *blurVertical;
	Sampler2DVulkan *readSampler;
	Texture2DVulkan *readImg;
	std::vector<VkDescriptorSet> swapChainImgDesc;
};

