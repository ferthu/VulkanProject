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
#include <sstream>

#undef main

void updateWinTitle(VulkanRenderer *rend);
void outPerfCounters(std::string params);

static std::vector<double>	perfCounter;
static double elapsedTime = 0.0;
static double lastElapsedTime = 0.0;
const double INIT_WAIT_TIMER = 100;
const int MIN_SAMPLES = 1000;
const double RUN_DURATION = INIT_WAIT_TIMER + 1000.f; //ms


int main(int argc, const char* argv)
{
	VulkanRenderer renderer;
	perfCounter.reserve(10000);
	uint32_t particles = 512 * 1024;
	uint32_t dimW = 1024, dimH = 1024, pixels = dimW * dimH;
	std::stringstream outString;
	outString << "ASYNC" << ", " << pixels << ", " << particles;
	renderer.initialize( new ComputeExperiment(ComputeExperiment::Mode::ASYNC, ComputeExperiment::MEM_LIMITED, particles), dimW, dimH, TRIPLE_BUFFERED); // 256, 256
	//renderer.initialize(new ComputeScene(ComputeScene::Mode::Blur), 512, 512, TRIPLE_BUFFERED);
	//renderer.initialize(new TriangleScene(), 512, 512, 0);
	//renderer.initialize(new ShadowScene(), 800, 600, 0);

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
		if (MIN_SAMPLES < perfCounter.size() && elapsedTime > RUN_DURATION)	break;
	}
	outPerfCounters(outString.str());
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

double mean(const std::vector<double> &data)
{
	double sum = 0;
	for (size_t i = 0; i < data.size(); i++)
		sum += data[i] / data.size();
	return sum;
}
double stdev(const std::vector<double> &data, double mean)
{
	double sum = 0;
	for (size_t i = 0; i < data.size(); i++)
	{
		double diff = (data[i] - mean);
		sum += diff*diff / data.size();
	}
	return std::sqrt(sum);
}

double sum(const std::vector<double> &data)
{
	double sum = 0;
	for (size_t i = 0; i < data.size(); i++)
		sum += data[i];
	return sum;
}

void outPerfCounters(std::string params)
{
	if (perfCounter.size() == 0) return;

	// Write perf counters into file
	std::ofstream stream("Perf.log", std::ios::app | std::ios::out);
	if (stream.is_open())
	{
		if (params.size() > 0)
			stream << params << ", ";

		double avg = mean(perfCounter);
		double dev = stdev(perfCounter, avg);
		stream << sum(perfCounter) << ", " << avg << ", " << dev << "\n";
		stream.close();
	}
}