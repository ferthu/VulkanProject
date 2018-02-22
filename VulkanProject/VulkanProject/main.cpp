#define VK_USE_PLATFORM_WIN32_KHR	// required for windows-specific vulkan structs and functions
#include "vulkan\vulkan.h"
#include "glm\glm.hpp"
#include "VulkanRenderer.h"
#include "VertexBufferVulkan.h"
#include "ShaderVulkan.h"

#undef main

int main(int argc, const char* argv)
{
	VulkanRenderer renderer;
	renderer.initialize();
	 
	// Create testing vertex buffer
	const uint32_t NUM_TRIS = 2;
	glm::vec4 testTriangles[NUM_TRIS * 3] = { 
		glm::vec4 { -0.5f, 0.1f, 0.5f, 0.0f }, 
		glm::vec4 { -0.2f, -0.1f, 0.5f, 0.0f },
		glm::vec4 { -0.7f, -0.1f, 0.5f, 0.0f },

		glm::vec4 { -0.5f, 0.1f, 0.5f, 0.0f },
		glm::vec4 { -0.7f, -0.1f, 0.5f, 0.0f },
		glm::vec4 { -0.2f, -0.1f, 0.5f, 0.0f },
	};
	VertexBufferVulkan* testVertexBuffer = new VertexBufferVulkan(&renderer, NUM_TRIS * 3 * sizeof(glm::vec4), VertexBufferVulkan::DATA_USAGE::STATIC);
	renderer.transferBufferData(testVertexBuffer->_bufferHandle, testTriangles, NUM_TRIS * 3 * sizeof(glm::vec4), 0);

	// Load precompiled shaders
	ShaderVulkan shaders("testShaders", &renderer);
	shaders.setShader("VertexShader.spv", ShaderVulkan::ShaderType::VS);
	shaders.setShader("FragmentShader.spv", ShaderVulkan::ShaderType::PS);
	std::string err;
	shaders.compileMaterial(err);

	renderer.simpleTrianglePipeline(testVertexBuffer, &shaders, 6);

	SDL_Event windowEvent;
	while (true)
	{
		if (SDL_PollEvent(&windowEvent))
		{
			if (windowEvent.type == SDL_QUIT) break;
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_ESCAPE) break;
		}

		renderer.frame();
		renderer.present();
	}

	delete testVertexBuffer;

	renderer.beginShutdown();
	renderer.shutdown();
}