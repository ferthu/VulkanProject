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

private:

	void makeTechniqueA();

	ShaderVulkan *computeShader;

	TechniqueVulkan * techniqueA;

	Sampler2DVulkan *readSampler;
	Texture2DVulkan *readImg;
};

