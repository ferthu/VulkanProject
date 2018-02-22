#define VK_USE_PLATFORM_WIN32_KHR	// required for windows-specific vulkan structs and functions
#include "vulkan\vulkan.h"
#include "glm\glm.hpp"
#include "VulkanRenderer.h"

#undef main

int main(int argc, const char* argv)
{
	VulkanRenderer renderer;
	renderer.initialize();

	renderer.beginShutdown();
	renderer.shutdown();
}