#pragma once
#include "..\Scene.h"

#include "vulkan\vulkan.h"
#include "glm\glm.hpp"
#include "VertexBufferVulkan.h"
#include "ShaderVulkan.h"
#include "Sampler2DVulkan.h"
#include "Texture2DVulkan.h"
#include "TechniqueVulkan.h"

class ShadowScene :
	public Scene
{
public:
	ShadowScene();
	virtual ~ShadowScene();

	virtual void frame(VkCommandBuffer cmdBuf);
	virtual void initialize(VulkanRenderer* handle);

	virtual void defineDescriptorLayout(VkDevice device, std::vector<VkDescriptorSetLayout> &layout);
	virtual VkRenderPass defineRenderPass(VkDevice device, VkFormat swapchainFormat, VkFormat depthFormat);

private:
	void createDescriptors();

	const uint32_t shadowMapBindingSlot = 0;
	const uint32_t lightMatrixBindingSlot = 1;
	const uint32_t cameraMatrixBindingSlot = 2;

	const uint32_t shadowMapSize = 512;

	glm::mat4 lightMatrix;
	glm::mat4 cameraMatrix;

	// Positions and normals of the triangles to render
	VertexBufferVulkan* positionBuffer;
	VertexBufferVulkan::Binding positionBufferBinding;
	VertexBufferVulkan* normalBuffer;
	VertexBufferVulkan::Binding normalBufferBinding;

	VkFramebuffer shadowMapFrameBuffer;
	Sampler2DVulkan* shadowMapSampler;
	Texture2DVulkan* shadowMap;
	VkViewport shadowMapViewport;

	TechniqueVulkan* depthPassTechnique;
	ShaderVulkan* depthPassShaders;

	TechniqueVulkan* renderPassTechnique;
	ShaderVulkan* renderPassShaders;

	VkFormat shadowMapFormat = VkFormat::VK_FORMAT_D16_UNORM;
};

// todo:
// descriptors + descriptor sets for matrices and shadow map
// write shaders
// set up rendering