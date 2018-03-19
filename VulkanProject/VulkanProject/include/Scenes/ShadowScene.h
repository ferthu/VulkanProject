#pragma once
#include "..\Scene.h"

#include "vulkan\vulkan.h"
#include "glm\glm.hpp"
#include "VertexBufferVulkan.h"
#include "ConstantBufferVulkan.h"
#include "ShaderVulkan.h"
#include "Sampler2DVulkan.h"
#include "Texture2DVulkan.h"
#include "TechniqueVulkan.h"

class ShadowScene :
	public Scene
{
public:
	enum FrameType
	{
		STANDARD, SINGLE_COMMAND_BUFFER, ASYNC
	};

	ShadowScene(FrameType frameType = STANDARD);
	virtual ~ShadowScene();

	virtual void frame(float dt);
	void frame_standard(float dt);
	void frame_single_cmdbuf(float dt);
	void frame_async(float dt);
	void post_async(float dt);
	void async_depthBuffer(float dt);

	virtual void transfer();
	virtual void initialize(VulkanRenderer* handle);

	virtual void defineDescriptorLayout(VkDevice device, std::vector<VkDescriptorSetLayout> &layout);
	virtual VkRenderPass defineRenderPass(VkDevice device, VkFormat swapchainFormat, VkFormat depthFormat, std::vector<VkImageView>& additionalAttatchments);
	void defineShadowRenderPass(VkDevice device);

private:
	glm::mat4 rotationMatrix(float angle, glm::vec3 const& axis);
	glm::mat4 orthographicMatrix(float left, float right, float bottom, float top, float near, float far);
	glm::mat4 perspectiveMatrix(float aspectRatio, float fov, float near, float far);
	// Rotate the camera based on time
	void createCameraMatrix(float time);

	void createBuffers();

	bool firstFrame;

	FrameType frameType;

	VkCommandBuffer depthCommandBuf[2];
	VkFence depthFence[2];
	VkSemaphore colorPassCompleteSemaphore;

	const uint32_t shadowMappingMatrixBindingSlot = 0;

	const uint32_t shadowMapBindingSlot = 0;
	const uint32_t transformMatrixBindingSlot = 1;
	const uint32_t clipToShadowMapMatrixBindingSlot = 2;

	const uint32_t shadowMapSize = 2048;

	glm::mat4 shadowMappingMatrix;			// Contains all transformations done in the shadow mapping pass
	ConstantBufferVulkan* shadowMappingMatrixBuffer;


	glm::mat4 transformMatrix;				// Contains all transformations done on the geometry in the rendering pass
	ConstantDoubleBufferVulkan* transformMatrixBuffer;

	struct LightInfo
	{
		glm::mat4 clipSpaceToShadowMapMatrix;
		glm::vec4 lightDirection;
	};
	LightInfo lightInfo;	// Transfoms a coordinate in clip space to a coordinate on shadow map
	ConstantDoubleBufferVulkan* lightInfoBuffer;

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

	VkDescriptorSet shadowPassDescriptorSet;
	VkDescriptorSet renderPassDescriptorSet;

	vk::LayoutConstruct shadowPipeLayout;

	VkDescriptorPool desciptorPool;

	VkRenderPass shadowRenderPass;
	VkFramebuffer shadowFramebuffer;


	// Post pass
	TechniqueVulkan *techniqueBlurHorizontal, *techniqueBlurVertical;
	vk::LayoutConstruct postLayout;
	ShaderVulkan *blurHorizontal, *blurVertical;
	std::vector<VkDescriptorSet> swapChainImgDesc;
};