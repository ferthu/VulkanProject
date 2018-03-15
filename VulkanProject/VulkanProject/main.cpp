#define VK_USE_PLATFORM_WIN32_KHR	// required for windows-specific vulkan structs and functions
#include "vulkan\vulkan.h"
#include "glm\glm.hpp"
#include "glm\gtc\matrix_transform.hpp"
#include "VulkanRenderer.h"
#include "Scenes/TriangleScene.h"
#include "Scenes/ComputeScene.h"
#include "Scenes/ComputeExperiment.h"
#include "Scenes/ShadowScene.h"

#define OBJ_READER_SIMPLE
#include "Stuff/ObjReaderSimple.h"
#undef main

void updateWinTitle(VulkanRenderer *rend);


int main(int argc, const char* argv)
{
	SimpleMesh mesh, baked;
	if (readObj("resource/Suzanne.obj", mesh))
		std::cout << "Obj read successfull\n";
	mesh.bake(SimpleMesh::BitFlag::NORMAL_BIT, baked);

	VulkanRenderer renderer;

	renderer.initialize(new ComputeExperiment(ComputeExperiment::Mode::SEQUENTIAL, ComputeExperiment::MEM_LIMITED, 1024 * 1024, 8), 1024, 1024, TRIPLE_BUFFERED); // 256, 256
	//renderer.initialize(new ComputeScene(ComputeScene::Mode::Blur), 512, 512, TRIPLE_BUFFERED);
	//renderer.initialize(new TriangleScene(), 512, 512, 0);
	//renderer.initialize(new ShadowScene(), 1024, 1024, 0);


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