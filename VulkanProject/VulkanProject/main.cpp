#define VK_USE_PLATFORM_WIN32_KHR	// required for windows-specific vulkan structs and functions
#include "vulkan\vulkan.h"
#include "glm\glm.hpp"
#include "glm\gtc\matrix_transform.hpp"
#include "VulkanRenderer.h"
#include "Scenes/TriangleScene.h"
#include "Scenes/ComputeScene.h"
#include "Scenes/ComputeExperiment.h"
#include "Scenes/ShadowScene.h"
#undef main

void updateWinTitle(VulkanRenderer *rend);


int main(int argc, const char* argv)
{
	VulkanRenderer renderer;

	//renderer.initialize( new ComputeExperiment(ComputeExperiment::Mode::MULTI_DISPATCH), 256, 256, TRIPLE_BUFFERED);
	//renderer.initialize(new ComputeScene(), 512, 512, TRIPLE_BUFFERED);
	//renderer.initialize(new TriangleScene(), 512, 512, 0);
	//glm::perspective(80.0f, 800.0f / 600.0f, 0.1f, 10.0f);
	glm::mat4 cameraMatrix = glm::mat4(1.0f);
	cameraMatrix = glm::translate(cameraMatrix, glm::vec3(0.0f, 0.0f, -2.0f));
	cameraMatrix = glm::rotate(cameraMatrix, glm::pi<float>() * 0.0f, glm::vec3(0, 1, 0));
	cameraMatrix = glm::perspective(2.0f, 600.0f / 800.0f, 0.1f, 20.0f) * cameraMatrix;

	glm::mat4 lightMatrix = glm::mat4(1.0f);
	lightMatrix = glm::translate(lightMatrix, glm::vec3(0.0f, 0.0f, -2.0f));
	lightMatrix = glm::rotate(lightMatrix, glm::pi<float>() * 0.0f, glm::vec3(0, 1, 0));
	lightMatrix = glm::orthoLH(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 10.0f) * lightMatrix;
	renderer.initialize(new ShadowScene(cameraMatrix, lightMatrix), 800, 600, 0);

	SDL_Event windowEvent;
	while (true)
	{
		if (SDL_PollEvent(&windowEvent))
		{
			if (windowEvent.type == SDL_QUIT) break;
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_ESCAPE) break;
		}
		renderer.frame();
		updateWinTitle(&renderer);
	}
	
	renderer.beginShutdown();
	renderer.shutdown();
}




void updateWinTitle(VulkanRenderer *rend)
{
#define WINDOW_SIZE 10
	static Uint64 start = 0;
	static Uint64 last = 0;
	static double avg[WINDOW_SIZE] = { 0.0 };
	static double lastSum = 10.0;
	static int loop = 0;
	static char gTitleBuff[256];
	static double gLastDelta = 0.0;

	last = start;
	start = SDL_GetPerformanceCounter();
	double deltaTime = (double)((start - last) * 1000.0 / SDL_GetPerformanceFrequency());
	// moving average window of WINDOWS_SIZE
	lastSum -= avg[loop];
	lastSum += deltaTime;
	avg[loop] = deltaTime;
	loop = (loop + 1) % WINDOW_SIZE;
	gLastDelta = (lastSum / WINDOW_SIZE);

	// Set title
	sprintf_s(gTitleBuff, 256, "%3.0lf", gLastDelta);
	rend->setWinTitle(gTitleBuff);
};