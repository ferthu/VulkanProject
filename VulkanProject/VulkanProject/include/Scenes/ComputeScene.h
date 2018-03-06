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
	ComputeScene();
	~ComputeScene();

	virtual void frame(VkCommandBuffer cmdBuf);
	virtual void initialize(VulkanRenderer *handle);
	virtual void defineDescriptorLayout(VkDevice device, std::vector<VkDescriptorSetLayout> &layout);
	virtual VkRenderPass defineRenderPass(VkDevice device, VkFormat swapchainFormat, VkFormat depthFormat, std::vector<VkImageView>& additionalAttatchments);

private:

	void makeTechnique();

	// Render pass
	ShaderVulkan *triShader;
	TechniqueVulkan * techniqueA;
	VertexBufferVulkan* triBuffer;
	VertexBufferVulkan::Binding triVertexBinding;

	// Post pass
	TechniqueVulkan * techniquePost;
	vk::LayoutConstruct postLayout;
	ShaderVulkan *compShader;
	Sampler2DVulkan *readSampler;
	Texture2DVulkan *readImg;
	std::vector<VkDescriptorSet> swapChainImgDesc;
};

