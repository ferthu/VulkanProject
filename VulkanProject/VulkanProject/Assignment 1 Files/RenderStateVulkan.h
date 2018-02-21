#pragma once
#include <vector>
#include "../RenderState.h"
#include <vulkan/vulkan.h>

class RenderStateVulkan : public RenderState
{
public:
	RenderStateVulkan();
	~RenderStateVulkan();
	void setWireFrame(bool);
	void set();

	void setGlobalWireFrame(bool* global);

	bool getWireframe();

	VkPipelineDynamicStateCreateInfo* getDynamicState();
private:
	bool wireframe;
	bool* globalWireframe;
};