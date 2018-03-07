#pragma once
#include "..\Scene.h"

#include "vulkan\vulkan.h"
#include "glm\glm.hpp"
#include "VertexBufferVulkan.h"
#include "ShaderVulkan.h"
#include "Texture2DVulkan.h"
#include "Sampler2DVulkan.h"


class ComputeExperiment :
	public Scene
{
public:
	
	enum Mode
	{
		REG_LIMITED,
		MEM_CONSUMPTION,
		SEQUENTIAL,
		MULTI_QUEUE,
		MULTI_DISPATCH
	};
	
	ComputeExperiment(Mode mode = REG_LIMITED, uint32_t num_particles = 1024 * 512);
	~ComputeExperiment();

	virtual void frame();
	virtual void initialize(VulkanRenderer *handle);
	virtual void defineDescriptorLayout(VkDevice device, std::vector<VkDescriptorSetLayout> &layout);
	virtual VkRenderPass defineRenderPass(VkDevice device, VkFormat swapchainFormat, VkFormat depthFormat, std::vector<VkImageView>& additionalAttatchments);

private:
	Mode mode;
	uint32_t NUM_PARTICLE;

	void makeTechnique();

	// Render pass
	ShaderVulkan *triShader;
	TechniqueVulkan * techniqueA;
	VertexBufferVulkan* triBuffer;
	VertexBufferVulkan::Binding triVertexBinding;

	// Post pass
	TechniqueVulkan * techniquePost, *techniqueSmallOp;
	vk::LayoutConstruct postLayout, smallOpLayout;
	ShaderVulkan *compShader, *compSmallOp;
	ConstantBufferVulkan *smallOpBuf;
	std::vector<VkDescriptorSet> swapChainImgDesc;
};

