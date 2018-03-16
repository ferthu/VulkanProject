#define VK_USE_PLATFORM_WIN32_KHR	// required for windows-specific vulkan structs and functions
#include "vulkan\vulkan.h"
#include "glm\glm.hpp"
#include "glm\gtc\matrix_transform.hpp"
#include "VulkanRenderer.h"
#include "Scenes/TriangleScene.h"
#include "Scenes/ComputeScene.h"
#include "Scenes/ComputeExperiment.h"
#include "Scenes/ShadowScene.h"
#include <vector>
#include <iostream>
#include <fstream>

#undef main

void updateWinTitle(VulkanRenderer *rend);
void outPerfCounters();

static std::vector<double>	perfCounter;
static double elapsedTime = 0.0;
static double lastElapsedTime = 0.0;
const double INIT_WAIT_TIMER = 100;
const double RUN_DURATION = INIT_WAIT_TIMER - 1000.f; //ms


int main(int argc, const char* argv)
{
	VulkanRenderer renderer;
	perfCounter.reserve(10000);

	//renderer.initialize( new ComputeExperiment(ComputeExperiment::Mode::ASYNC, ComputeExperiment::MEM_LIMITED | ComputeExperiment::MEM_LIMITED_ANIMATED, 1024 * 512), 1024, 1024, TRIPLE_BUFFERED); // 256, 256
	//renderer.initialize(new ComputeScene(ComputeScene::Mode::Blur), 512, 512, TRIPLE_BUFFERED);
	//renderer.initialize(new TriangleScene(), 512, 512, 0);
	renderer.initialize(new ShadowScene(), 800, 600, 0);

	SDL_Event windowEvent;
	while (true)
	{
		if (SDL_PollEvent(&windowEvent))
		{
			if (windowEvent.type == SDL_QUIT) break;
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_ESCAPE) break;
		}
		renderer.frame(static_cast<float>(elapsedTime - lastElapsedTime) / 1000.0f);
		lastElapsedTime = elapsedTime;
		updateWinTitle(&renderer);
		if (RUN_DURATION > 0 && elapsedTime > RUN_DURATION)	break;
	}
	outPerfCounters();
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
	if (last != 0)
	{
		elapsedTime = std::fmod(deltaTime + elapsedTime, 100000);
		if (elapsedTime > INIT_WAIT_TIMER)
			perfCounter.push_back(deltaTime);
	}
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

void outPerfCounters()
{
	if (perfCounter.size() == 0) return;

	// Write perf counters into file
	std::ofstream stream("Perf.log");
	if (stream.is_open())
	{
		stream << "#Elapsed, Delta\n";
		double elapsedTime = 0;
		for (int i = 0; i < (int)perfCounter.size(); i++)
		{
			stream << elapsedTime << ", " << perfCounter[i] << "\n";
			elapsedTime += perfCounter[i];
		}
		stream.close();
	}
}