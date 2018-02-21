#include "RenderStateVulkan.h"

RenderStateVulkan::RenderStateVulkan()
{
	wireframe = false;
	globalWireframe = nullptr;
}

RenderStateVulkan::~RenderStateVulkan()
{
}

void RenderStateVulkan::setWireFrame(bool wireframe)
{
	this->wireframe = wireframe;
}

void RenderStateVulkan::set()
{
	if (globalWireframe == nullptr || wireframe == *globalWireframe)
		return;
	else
		*globalWireframe = wireframe;

	// Set dynamic render states...
}

void RenderStateVulkan::setGlobalWireFrame(bool * global)
{
	globalWireframe = global;
}

bool RenderStateVulkan::getWireframe()
{
	return wireframe || *globalWireframe;
}


VkPipelineDynamicStateCreateInfo* RenderStateVulkan::getDynamicState()
{
	//Todo: make dynamic settings...
	return nullptr;
}