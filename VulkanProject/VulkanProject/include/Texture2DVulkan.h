#pragma once

#include <vulkan/vulkan.h>
#include <string>

class Sampler2DVulkan;
class VulkanRenderer;

const int MAX_TEX_BINDINGS = 6;

class Texture2DVulkan 
{
private:
	void destroyImg();
	VkDescriptorSet slotBindings[MAX_TEX_BINDINGS];	// Set of descriptors associated with the image.

public:
	Texture2DVulkan(VulkanRenderer *renderer, Sampler2DVulkan *sampler);
	~Texture2DVulkan();

	int loadFromFile(std::string filename);
	/* Bind an attached texture descriptor to the certain set slot.
	combinedIndex	<<	Both the set binding the descriptor is bound to and the index for the attached descriptor bound.
	*/
	void bind(VkCommandBuffer cmdBuf, uint32_t combinedIndex, VkPipelineLayout layout, VkPipelineBindPoint pipeBinding = VK_PIPELINE_BIND_POINT_GRAPHICS);
	/* Bind an attached texture descriptor to the certain set slot.
	attachmentIndex	<<	The texture attachment slot the descriptor was generated for.
	setIndex		<<	The set binding the descriptor is bound to.
	*/
	void bind(VkCommandBuffer cmdBuf, uint32_t attachmentIndex, uint32_t setIndex, VkPipelineLayout layout, VkPipelineBindPoint pipeBinding = VK_PIPELINE_BIND_POINT_GRAPHICS);
	/* Generate a descriptor at the specific attachment index.
	*/
	void attachBindPoint(uint32_t attachmentIndex, VkDescriptorSetLayout layout);
	void createShadowMap(uint32_t height, uint32_t width, VkFormat shadowMapFormat);
	void bind(VkCommandBuffer cmdBuf, unsigned int slot);

	VulkanRenderer *_renderHandle;
	Sampler2DVulkan *_samplerHandle;
	VkImage _imageHandle;
	VkDescriptorImageInfo imageInfo;
};

