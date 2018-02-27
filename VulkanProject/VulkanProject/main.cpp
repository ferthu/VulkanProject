#define VK_USE_PLATFORM_WIN32_KHR	// required for windows-specific vulkan structs and functions
#include "vulkan\vulkan.h"
#include "glm\glm.hpp"
#include "VulkanRenderer.h"
#include "Scenes/TriangleScene.h"
#include "Scenes/ComputeScene.h"
#undef main

int main(int argc, const char* argv)
{
	VulkanRenderer renderer;
	renderer.initialize( new ComputeScene(), 640, 640);

	SDL_Event windowEvent;
	while (true)
	{
		if (SDL_PollEvent(&windowEvent))
		{
			if (windowEvent.type == SDL_QUIT) break;
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_ESCAPE) break;
		}
		renderer.frame();
	}
	
	renderer.beginShutdown();
	renderer.shutdown();
}