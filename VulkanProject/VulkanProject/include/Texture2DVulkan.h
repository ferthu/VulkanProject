#pragma once

#include <GL/glew.h>

#include "../Texture2D.h"
#include "Sampler2DVulkan.h"
#include <vulkan/vulkan.h>

class VulkanRenderer;
class Material;

const int MAX_TEX_BINDINGS = 6;

class Texture2DVulkan :
	public Texture2D
{
private:
	void destroyImg();
	VkDescriptorSet slotBindings[MAX_TEX_BINDINGS];

public:
	Texture2DVulkan(VulkanRenderer *renderer);
	~Texture2DVulkan();

	int loadFromFile(std::string filename);
	void bind(unsigned int slot);

	VulkanRenderer *_renderHandle;
	VkImage _imageHandle;
	VkDescriptorImageInfo imageInfo;
};

