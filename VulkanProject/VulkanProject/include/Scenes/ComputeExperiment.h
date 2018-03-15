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
		ASYNC,
		SEQUENTIAL,
		MULTI_QUEUE,
		MULTI_DISPATCH
	};

	enum ShaderModeBit
	{
		MEM_LIMITED = 1,
		MEM_LIMITED_ANIMATED = 2,
		REG_LIMITED = 4
	};
	
	ComputeExperiment(Mode mode = ASYNC, uint32_t shader = REG_LIMITED, uint32_t num_particles = 1024 * 512, float locality = 8);
	~ComputeExperiment();

	virtual void frame();
	virtual void transfer();
	virtual void initialize(VulkanRenderer *handle);
	virtual void defineDescriptorLayout(VkDevice device, std::vector<VkDescriptorSetLayout> &layout);
	virtual VkRenderPass defineRenderPass(VkDevice device, VkFormat swapchainFormat, VkFormat depthFormat, std::vector<VkImageView>& additionalAttatchments);

private:
	Mode mode;
	uint32_t shaderMode;
	uint32_t NUM_PARTICLE;
	float locality;

	void makeTechnique();

	// Render pass
	ShaderVulkan *triShader;
	TechniqueVulkan * techniqueA;
	VertexBufferVulkan* triBuffer;
	VertexBufferVulkan::Binding triVertexBinding;

	// Post pass
	TechniqueVulkan *techniquePost, *techniqueSmallOp;
	vk::LayoutConstruct postLayout, smallOpLayout;
	ShaderVulkan *compShader, *compSmallOp;
	ConstantBufferVulkan *smallOpBuf;
	ConstantDoubleBufferVulkan *postParams;
	
	Sampler2DVulkan *readSampler;
	Texture2DVulkan *readImg;

	std::vector<VkDescriptorSet> swapChainImgDesc;
};

