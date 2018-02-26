#pragma once
#include "..\Scene.h"

#include "vulkan\vulkan.h"
#include "glm\glm.hpp"
#include "VertexBufferVulkan.h"
#include "ShaderVulkan.h"

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

	ShaderVulkan *triShader;

	TechniqueVulkan * techniqueA;
	VertexBufferVulkan* triBuffer;

	VertexBufferVulkan::Binding triVertexBinding;
};

