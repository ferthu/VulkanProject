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
void resetTime();
void outPerfCounters(std::string params);

static std::vector<double>	perfCounter, graphQueue, compQueue1, compQueue2;
static double elapsedTime = 0.0;
static double lastElapsedTime = 0.0;
const double INIT_WAIT_TIMER = 100;
const int MIN_SAMPLES = 0;								// = 0 if inf runtime, sampling: 500
const double RUN_DURATION = INIT_WAIT_TIMER + 1000.f;	//ms

inline double square(double val) { return val * val; }
inline uint32_t square(uint32_t val) { return val * val; }

const std::string MODE_STR[] = {
	"ASYNC",
	"SEQ",
	"MQUEUE",
	"MULTI_DISPATCH"
};

int main(int argc, const char* argv)
{
	perfCounter.reserve(10000);
	graphQueue.reserve(10000);
	compQueue1.reserve(10000);
	compQueue2.reserve(10000);
	uint32_t RUN_ONCE = 1;
	uint32_t LOCALITY_TESTS = 14; 

	uint32_t ITERS = 16;

	for (uint32_t i = 0; i < (MIN_SAMPLES == 0 ? 1 : ITERS); i++)
	{
		resetTime();
		elapsedTime = 0.0;
		lastElapsedTime = 0.0;
		perfCounter.clear();
		graphQueue.clear();
		compQueue1.clear();
		compQueue2.clear();

		VulkanRenderer renderer;
		uint32_t particles = 1024 *  64 * (i + 33);
		uint32_t dimW = 1024, dimH = 1024;//64 * (i + 1);
		uint32_t pixels = dimW * dimH;
		float locality = 8.f; // 0.25f * (std::pow(2.f, i*0.4f));
		uint32_t mode = ComputeExperiment::Mode::MULTI_QUEUE;
		uint32_t shader = ComputeExperiment::MEM_LIMITED;
		std::stringstream outString;
		outString << "MEM_" << MODE_STR[mode] << ", " << pixels << ", " << particles << ", " << locality;
		renderer.initialize(new ComputeExperiment((ComputeExperiment::Mode)mode, shader, particles, locality), dimW, dimH, TRIPLE_BUFFERED); // 256, 256
		//renderer.initialize(new ComputeScene(ComputeScene::Mode::Blur), 1024, 1024, 0);
		//renderer.initialize(new TriangleScene(), 512, 512, 0);
		//renderer.initialize(new ShadowScene(), 800, 600, TRIPLE_BUFFERED);

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
			if (MIN_SAMPLES != 0 && MIN_SAMPLES < perfCounter.size() && elapsedTime > RUN_DURATION)	break;
		}
		outPerfCounters(outString.str());
		renderer.beginShutdown();
		renderer.shutdown();
	}
}


static Uint64 start = 0;
static Uint64 last = 0;

void resetTime()
{
	start = 0;
	last = 0;
}

void updateWinTitle(VulkanRenderer *rend)
{
#define WINDOW_SIZE 10
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
		
		if (rend->_queries._numQueries > 0)
			graphQueue.push_back(rend->_queries.getTimestampDiff(0));
		if (rend->_queries._numQueries > 2)
			compQueue1.push_back(rend->_queries.getTimestampDiff(2));
		if (rend->_queries._numQueries > 4)
			compQueue2.push_back(rend->_queries.getTimestampDiff(4));
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
		stream << sum(perfCounter) << ", " << avg << ", " << dev;
		if (graphQueue.size() > 0)
		{
			avg = mean(graphQueue);
			dev = stdev(graphQueue, avg);
			stream << ", " << avg << ", " << dev;
		}
		if (compQueue1.size() > 0)
		{
			avg = mean(compQueue1);
			dev = stdev(compQueue1, avg);
			stream << ", " << avg << ", " << dev;
		}
		if (compQueue2.size() > 0)
		{
			avg = mean(compQueue2);
			dev = stdev(compQueue2, avg);
			stream  << ", " << avg << ", " << dev;
		}
		stream << std::endl;
		stream.close();
	}
	stream.open("Metric.log", std::ios::trunc | std::ios::out);
	if (stream.is_open())
	{
		double elapsed = 0;
		stream << "#" << params << "\n";
		stream << "#Elapsed	FrameTime\n";
		for (size_t i = 0; i < perfCounter.size(); i++)
		{
			elapsed += perfCounter[i];
			stream << elapsed << ", " << perfCounter[i] << "\n";
		}
		stream.close();
	}
}