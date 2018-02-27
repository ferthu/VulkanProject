#pragma once
#include "..\Scene.h"

#include "vulkan\vulkan.h"
#include "glm\glm.hpp"
#include "VertexBufferVulkan.h"
#include "ShaderVulkan.h"

class TriangleScene :
	public Scene
{
public:
	TriangleScene();
	virtual ~TriangleScene();


	virtual void frame(VkCommandBuffer cmdBuf);
	virtual void initialize(VulkanRenderer *handle);
	virtual void defineDescriptorLayout(VkDevice device, std::vector<VkDescriptorSetLayout> &layout);
	virtual VkRenderPass defineRenderPass(VkDevice device, VkFormat swapchainFormat, VkFormat depthFormat);

private:

	void makeTechnique();

	// Render pass
	ShaderVulkan *triShader;
	TechniqueVulkan * techniqueA;
	VertexBufferVulkan* triBuffer;
	VertexBufferVulkan::Binding triVertexBinding;


};

