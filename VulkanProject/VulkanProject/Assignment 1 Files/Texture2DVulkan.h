#pragma once

#include <GL/glew.h>

#include "../Texture2D.h"
#include "Sampler2DVulkan.h"
#include <vulkan/vulkan.h>

class VulkanRenderer;
class Material;

class Texture2DVulkan :
	public Texture2D
{
private:
	void destroyImg();

	unsigned int last_slot = 100000;

public:
	Texture2DVulkan(VulkanRenderer *renderer);
	~Texture2DVulkan();

	int loadFromFile(std::string filename);
	void bind(unsigned int slot, Material *m);

	VulkanRenderer *_renderHandle;
	VkImage _imageHandle;
	VkDescriptorImageInfo imageInfo;
	VkDescriptorSet descriptor;
};

