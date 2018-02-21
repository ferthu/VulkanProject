#pragma once
#pragma once
#include "Technique.h"
#include "vulkan\vulkan.h"
#include "Vulkan\VulkanRenderer.h"
#include <map>

class Renderer;



class TechniqueVulkan : public Technique
{
public:
	TechniqueVulkan(VulkanMaterial* m, RenderState* r, VulkanRenderer* renderer, VkRenderPass renderPass);
	virtual ~TechniqueVulkan();
	VulkanMaterial* getMaterial() { return material; };
	RenderState* getRenderState() { return renderState; };
	virtual void enable(Renderer* renderer);

	VkPipeline pipeline;

private:
	void createPipeline();

	VulkanRenderer* _renderHandle;
	VkRenderPass _passHandle;
	
	
};

