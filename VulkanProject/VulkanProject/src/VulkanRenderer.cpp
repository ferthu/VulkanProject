#include <vector>

#include "VulkanRenderer.h"
#include "ShaderVulkan.h"
#include "VertexBufferVulkan.h"
#include "Texture2DVulkan.h"
#include "Sampler2DVulkan.h"
#include "ConstantBufferVulkan.h"
#include "TechniqueVulkan.h"
#include <SDL/SDL_syswm.h>
#include <assert.h>
#include <iostream>
#include "Scene.h"


VulkanRenderer::VulkanRenderer()
 : memPool((int)MemoryPool::Count)
{
}
VulkanRenderer::~VulkanRenderer() { }


#pragma region Init & Destroy

int VulkanRenderer::initialize(Scene *scene, unsigned int width, unsigned int height)
{
	this->scene = scene;
	swapchainExtent.height = height;
	swapchainExtent.width = width;

	viewport.x = 0;
	viewport.y = 0;
	viewport.height = (float)height;
	viewport.width = (float)width;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	/* Create Vulkan instance
	*/

	VkApplicationInfo applicationInfo = {};
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.pNext = nullptr;
	applicationInfo.pApplicationName = "Basic Vulkan Renderer";
	applicationInfo.applicationVersion = 1;
	applicationInfo.pEngineName = "";
	applicationInfo.apiVersion = 0;		// setting to 1 causes error when creating instance

	std::vector<const char*> enabledLayers = {
#ifdef _DEBUG
		"VK_LAYER_LUNARG_standard_validation"
#endif
	};
	std::vector<const char*> availableLayers = checkValidationLayerSupport(enabledLayers.data(), enabledLayers.size());

	std::vector<const char*> enabledExtensions =
	{
		"VK_KHR_surface",
		"VK_KHR_win32_surface",
#ifdef _DEBUG
		"VK_EXT_debug_report"
#endif
	};
	VkInstanceCreateInfo instanceCreateInfo = {};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pNext = nullptr;
	instanceCreateInfo.flags = 0;
	instanceCreateInfo.pApplicationInfo = &applicationInfo;
	instanceCreateInfo.enabledLayerCount = (uint32_t)availableLayers.size();
	instanceCreateInfo.ppEnabledLayerNames = availableLayers.data();
	instanceCreateInfo.enabledExtensionCount = (uint32_t)enabledExtensions.size();
	instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();

	// Create instance
	VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create Vulkan instance");

	/* Create window
	*/

	// Initiate SDL
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		fprintf(stderr, "%s", SDL_GetError());
		exit(-1);
	}

	// Create window
	window = SDL_CreateWindow("Vulkan", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);

	/* Create window surface
	Note* Surface 'must' be created before physical device creation as it influences the selection.
	*/

	// Get the window version
	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);
	if (SDL_GetWindowWMInfo(window, &info) != SDL_TRUE)
		throw std::runtime_error("Window errror");

	// Utilize version to create the window surface
	VkWin32SurfaceCreateInfoKHR w32sci = {};
	w32sci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	w32sci.pNext = NULL;
	w32sci.hinstance = GetModuleHandle(NULL);
	w32sci.hwnd = info.info.win.window;
	VkResult err = vkCreateWin32SurfaceKHR(instance, &w32sci, nullptr, &windowSurface);
	assert(err == VK_SUCCESS);


	/* Create physical device
	*/

	VkQueueFlags queueSupport = (VK_QUEUE_TRANSFER_BIT | VK_QUEUE_GRAPHICS_BIT);
	chosenPhysicalDevice = choosePhysicalDevice(instance, windowSurface, vk::specifyAnyDedicatedDevice, queueSupport, physicalDevice);
	if (chosenPhysicalDevice < 0)
		throw std::runtime_error("No available physical device matched specification.");

	/* Create logical device
	*/

	// Find a suitable queue families
	VkQueueFlags prefGraphQueue[] =
	{
		(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)
	};
	VkQueueFlags prefComputeQueue[] =
	{
		(VK_QUEUE_COMPUTE_BIT)
	};
	VkQueueFlags prefMemQueue[] =
	{
		(VK_QUEUE_TRANSFER_BIT)
	};
	queues = vk::QueueConstruct(QueueType::COUNT);
	queues.push_queue(physicalDevice, 1.f, prefMemQueue, (uint32_t)std::size(prefMemQueue));
	queues.push_queue(physicalDevice, 1.f, prefGraphQueue, (uint32_t)std::size(prefGraphQueue));
	queues.push_queue(physicalDevice, 1.f, prefComputeQueue, (uint32_t)std::size(prefComputeQueue));

	// Info on queues
	VkDeviceQueueCreateInfo queueInfo[QueueType::COUNT] = {};
	uint32_t num_queue_families = QueueType::COUNT;
	float prioArr[QueueType::COUNT];
	queues.define(queueInfo, num_queue_families, prioArr);
	// Info on device
	const char* deviceLayers[] = { "VK_LAYER_LUNARG_standard_validation" };
	const char* deviceExtensions[] = { "VK_KHR_swapchain" };

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pNext = nullptr;
	deviceCreateInfo.flags = 0;
	deviceCreateInfo.queueCreateInfoCount = num_queue_families;
	deviceCreateInfo.pQueueCreateInfos = queueInfo;
#
#ifdef _DEBUG
	deviceCreateInfo.enabledLayerCount = 1;
	deviceCreateInfo.ppEnabledLayerNames = deviceLayers;
#else
	deviceCreateInfo.enabledLayerCount = 0;
	deviceCreateInfo.ppEnabledLayerNames = nullptr;
#endif

	deviceCreateInfo.enabledExtensionCount = 1;
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

	// Features
	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.fillModeNonSolid = true;
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

	// Create (vulkan) device
	err = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);
	if (err)
		throw std::runtime_error("Failed to create device...");
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, windowSurface, &surfaceCapabilities);
	if (err)
		throw std::runtime_error("Failed to acquire surface capabilities...");
	if (!hasFlag(surfaceCapabilities.supportedUsageFlags, VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT))
		throw std::runtime_error("Swapchain images does not support storage...");
	else
		std::cout << "Swapchain storage supported!\n";

	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

	queues.fetchDeviceQueues(device);
#
#ifdef _DEBUG
	checkValidImageFormats(physicalDevice);
#endif

	/* Create swap chain
	*/

	// Check that queue supports presenting
	VkBool32 presentingSupported;
	err = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queues[QueueType::GRAPHIC].family, windowSurface, &presentingSupported);
	if (err)
		throw std::runtime_error("Failed to acquire surface support...");


	if (presentingSupported == VK_FALSE)
		throw std::runtime_error("The selected queue does not support presenting. Do more programming >:|");

	// Get supported formats
	std::vector<VkSurfaceFormatKHR> formats;
	ALLOC_QUERY_ASSERT(result, vkGetPhysicalDeviceSurfaceFormatsKHR, formats, physicalDevice, windowSurface);
	swapchainFormat = formats[0]; // Just select the first available format

	// Choose the mode for the swap chain that determines how the frame buffers are swapped.
	VkPresentModeKHR presentModePref[] =
	{
#ifdef NO_VSYNC
		VK_PRESENT_MODE_IMMEDIATE_KHR,// Immediately present images to screen
#endif
		VK_PRESENT_MODE_MAILBOX_KHR // Oldest finished frame are replaced if 'framebuffer' queue is filled.
	};
	VkPresentModeKHR presentMode = chooseSwapPresentMode(physicalDevice, windowSurface, presentModePref, std::size(presentModePref));

	// Create swap chain
	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.pNext = nullptr;
	swapchainCreateInfo.flags = 0;
	swapchainCreateInfo.surface = windowSurface;
	swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount;
	swapchainCreateInfo.imageFormat = swapchainFormat.format;	
	swapchainCreateInfo.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	swapchainCreateInfo.imageExtent = swapchainExtent;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.presentMode = presentMode;
	swapchainCreateInfo.clipped = VK_FALSE;
	swapchainCreateInfo.oldSwapchain = NULL;

	result = vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create swapchain");

	// Aquire the swapchain images
	ALLOC_QUERY_ASSERT(result, vkGetSwapchainImagesKHR, swapchainImages, device, swapchain);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to get swapchain images");

	// Create image views for the swapchain images
	swapchainImageViews.resize(swapchainImages.size());
	for (int i = 0; i < swapchainImages.size(); ++i)
		swapchainImageViews[i] = createImageView(device, swapchainImages[i], swapchainCreateInfo.imageFormat);

	// Allocate device memory
	createStagingBuffer();
	// Create command pools
	queues.createCommandPool(device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	// Allocate device memory
	allocateImageMemory(MemoryPool::IMAGE_RGBA8_BUFFER, STORAGE_SIZE[MemoryPool::IMAGE_RGBA8_BUFFER], VK_FORMAT_R8G8B8A8_UNORM);
	allocateImageMemory(MemoryPool::IMAGE_D16_BUFFER, STORAGE_SIZE[MemoryPool::IMAGE_D16_BUFFER], VK_FORMAT_D16_UNORM);
	allocateBufferMemory(MemoryPool::UNIFORM_BUFFER, STORAGE_SIZE[MemoryPool::UNIFORM_BUFFER], VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	allocateBufferMemory(MemoryPool::VERTEX_BUFFER, STORAGE_SIZE[MemoryPool::VERTEX_BUFFER], VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	allocateBufferMemory(MemoryPool::INDEX_BUFFER, STORAGE_SIZE[MemoryPool::INDEX_BUFFER], VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
	createDepthComponents();

	// Create render pass
	frameBufferPass = scene->defineRenderPass(device, swapchainCreateInfo.imageFormat, depthFormat);
	
	// Create frame buffers.
	const uint32_t NUM_FRAME_ATTACH = 2;
	swapChainFramebuffers.resize(swapchainImages.size());
	for (size_t i = 0; i < swapChainFramebuffers.size(); i++)
	{
		VkImageView attachments[NUM_FRAME_ATTACH] = {
			swapchainImageViews[i],
			depthImageView
		};
		swapChainFramebuffers[i] = createFramebuffer(device, frameBufferPass, swapchainExtent, attachments, NUM_FRAME_ATTACH);
	}


	// Stuff
	imageAvailable = createSemaphore(device);
	renderFinished[0] = createSemaphore(device);
	renderFinished[1] = createSemaphore(device);
	computeFinished = createSemaphore(device);
	_transferFences[0] = createFence(device, true);
	_transferFences[1] = createFence(device, false);


	for (uint32_t i = 0; i < MAX_DESCRIPTOR_POOLS; i++)
		descriptorPools[i] = NULL;
	descriptorPools[VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER]
		= createDescriptorPoolSingle(device, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2000);
	descriptorPools[VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER]
		= createDescriptorPoolSingle(device, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2000);
	descriptorPools[VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER]
		= createDescriptorPoolSingle(device, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2000);
	descriptorPools[VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE]
		= createDescriptorPoolSingle(device, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2000);

	//Begin initial transfer command
	_transferCmd[0] = allocateCmdBuf(device, queues[QueueType::MEM].pool);
	_transferCmd[1] = allocateCmdBuf(device, queues[QueueType::MEM].pool);
	_frameCmdBuf[0] = allocateCmdBuf(device, queues[QueueType::GRAPHIC].pool);
	_frameCmdBuf[1] = allocateCmdBuf(device, queues[QueueType::GRAPHIC].pool);
	_computeCmdBuf = allocateCmdBuf(device, queues[QueueType::COMPUTE].pool);

	// Our scene implementation
	scene->defineDescriptorLayout(device, descriptorLayouts);
	pipelineLayout = createPipelineLayout(device, descriptorLayouts.data(), (uint32_t)descriptorLayouts.size());
	this->scene->initialize(this);

	beginCmdBuf(_transferCmd[getTransferIndex()]);


	return 0;
}


int VulkanRenderer::beginShutdown()
{
	endSingleCommand_Wait(device, queues[QueueType::MEM].queue, queues[QueueType::MEM].pool, _transferCmd[getTransferIndex()]);
	releaseCommandBuffer(device, queues[QueueType::MEM].queue, queues[QueueType::MEM].pool, _transferCmd[getFrameIndex()]);
	releaseCommandBuffer(device, queues[QueueType::GRAPHIC].queue, queues[QueueType::GRAPHIC].pool, _frameCmdBuf, 2);
	// Wait for device to finish before shuting down..
	vkDeviceWaitIdle(device);
	return 0;
}

int VulkanRenderer::shutdown()
{
	delete scene;

	// Clean up Vulkan
	for (unsigned int i = 0; i < MAX_DESCRIPTOR_POOLS; i++)
	{
		if(descriptorPools[i])
			vkDestroyDescriptorPool(device, descriptorPools[i], nullptr);
	}
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	for (size_t i = 0; i < descriptorLayouts.size(); i++)
			vkDestroyDescriptorSetLayout(device, descriptorLayouts[i], nullptr);

	// Destroy command pools
	queues.destroy(device);

	vkDestroySemaphore(device, renderFinished[0], nullptr);
	vkDestroySemaphore(device, renderFinished[1], nullptr);
	vkDestroySemaphore(device, computeFinished, nullptr);
	vkDestroySemaphore(device, imageAvailable, nullptr);
	vkDestroyFence(device, _transferFences[0], nullptr);
	vkDestroyFence(device, _transferFences[1], nullptr);
	vkDestroyBuffer(device, stagingBuffer, nullptr);

	// Destroy frame buffer
	vkDestroyImageView(device, depthImageView, nullptr);
	vkDestroyImage(device, depthImage, nullptr);
	for (auto framebuffer : swapChainFramebuffers) 
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	for (int i = 0; i < swapchainImageViews.size(); ++i)
		vkDestroyImageView(device, swapchainImageViews[i], nullptr);

	// Clear memory
	for(uint32_t i = 0; i < memPool.size(); i++)
		vkFreeMemory(device, memPool[i].handle, nullptr);


	vkDestroySwapchainKHR(device, swapchain, nullptr);
	vkDestroyRenderPass(device, frameBufferPass, nullptr);
	vkDestroySurfaceKHR(instance, windowSurface, nullptr);
	vkDestroyDevice(device, nullptr);
	vkDestroyInstance(instance, nullptr);

	//Clean up SDL
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;	// temp
}

#pragma endregion


void VulkanRenderer::clearBuffer(unsigned int flag)
{

}

#pragma region Frame

void VulkanRenderer::present(bool waitRender, bool waitCompute)
{
	// Present

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	// Synchronization:
	VkSemaphore wait[2];
	uint32_t numWait = 0;
	if (waitRender)
		wait[numWait++] = renderFinished[getFrameIndex()];
	if (waitCompute)
		wait[numWait++] = computeFinished;
	presentInfo.waitSemaphoreCount = numWait;
	presentInfo.pWaitSemaphores = wait;

	VkSwapchainKHR swapChains[] = { swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &swapChainImgIndex;
	presentInfo.pResults = nullptr; // Optional
	vkQueuePresentKHR(queues[QueueType::GRAPHIC].queue, &presentInfo);


	nextFrame();
}

void VulkanRenderer::nextFrame()
{
	// Cycle frame index
	frameCycle = !frameCycle;	
	// Reset the command buffer
	VkResult err = vkResetCommandBuffer(_transferCmd[getTransferIndex()], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
	if (err)
		std::cout << "Command buff reset err\n";
	// Start recording new transfer commands
	beginCmdBuf(_transferCmd[getTransferIndex()]);
}

VulkanRenderer::FrameInfo VulkanRenderer::beginFramePass(VkFramebuffer* frameBuffer)
{
	VkCommandBuffer cmdBuf = _frameCmdBuf[getFrameIndex()];
	VkResult err = vkResetCommandBuffer(cmdBuf, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
	if (err)
		std::cout << "Command buff reset err\n";
	// Begin recording frame commands
	beginCmdBuf(cmdBuf, VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT | VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	//Render pass
	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = frameBufferPass;
	renderPassInfo.framebuffer = (frameBuffer == nullptr) ? swapChainFramebuffers[swapChainImgIndex] : *frameBuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = swapchainExtent;

	// Clear params
	const uint32_t num_clear_values = 2;
	VkClearValue clearValues[num_clear_values];
	clearValues[0].color = { this->clearColor.r, this->clearColor.g, this->clearColor.b, this->clearColor.a };
	clearValues[1].depthStencil = { 1.0f, 0 };
	renderPassInfo.clearValueCount = num_clear_values;
	renderPassInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass(cmdBuf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	FrameInfo info;
	info._buf = cmdBuf;
	info._swapChainIndex = swapChainImgIndex;
	info._swapChainImage = swapchainImages[swapChainImgIndex];
	return info;
}

void VulkanRenderer::submitFramePass()
{

	VkCommandBuffer cmdBuf = _frameCmdBuf[getFrameIndex()];
	vkCmdEndRenderPass(cmdBuf);
	if (vkEndCommandBuffer(cmdBuf) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}

	// Submit
	VkSemaphore waitSemaphores[] = { imageAvailable };
	VkSemaphore signalSemaphores[] = { renderFinished[getFrameIndex()] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;	// Mask stage where related semaphore is waited for.
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuf;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;
	VkResult err = vkQueueSubmit(queues[QueueType::GRAPHIC].queue, 1, &submitInfo, VK_NULL_HANDLE);
	if (err != VK_SUCCESS) {
		throw std::runtime_error("Failed to submit draw command buffer!");
	}

}

VulkanRenderer::FrameInfo VulkanRenderer::beginCompute()
{ 
	vkQueueWaitIdle(queues[QueueType::COMPUTE].queue);
	VkResult err = vkResetCommandBuffer(_computeCmdBuf, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
	if (err)
		std::cout << "Command buff reset err\n";
	// Begin recording frame commands
	beginCmdBuf(_computeCmdBuf,  VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	FrameInfo info;
	info._buf = _computeCmdBuf;
	info._swapChainIndex = swapChainImgIndex;
	info._swapChainImage = swapchainImages[swapChainImgIndex];
	return info;
}
void VulkanRenderer::submitCompute()
{
	if (vkEndCommandBuffer(_computeCmdBuf) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}

	// Submit
	VkSemaphore waitSemaphores[] = { renderFinished[getFrameIndex()] };
	VkSemaphore signalSemaphores[] = { computeFinished };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = (uint32_t)std::size(waitSemaphores);
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;		// Mask stage where related semaphore is waited for.
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &_computeCmdBuf;
	submitInfo.signalSemaphoreCount = (uint32_t)std::size(signalSemaphores);
	submitInfo.pSignalSemaphores = signalSemaphores;
	VkResult err = vkQueueSubmit(queues[QueueType::COMPUTE].queue, 1, &submitInfo, VK_NULL_HANDLE);
	if (err != VK_SUCCESS) {
		throw std::runtime_error("Failed to submit draw command buffer!");
	}
}


void VulkanRenderer::frame()
{
	// Submit new transfer commands
	endSingleCommand(device, queues[QueueType::MEM].queue, _transferCmd[getTransferIndex()], _transferFences[getTransferIndex()]);
	// Wait for previous transfer frame to complete before using it for rendering! 
	waitFence(device, _transferFences[getFrameIndex()]);

	// Start rendering
	vkAcquireNextImageKHR(device, swapchain, std::numeric_limits<uint64_t>::max(), imageAvailable, VK_NULL_HANDLE, &swapChainImgIndex);



	// Draw stuff..?:)
	scene->frame();


}


#pragma endregion



void VulkanRenderer::createDepthComponents()
{
	depthFormat = findDepthFormat(physicalDevice);
	depthImage = createDepthBuffer(device, swapchainExtent.width, swapchainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL);
	bindPhysicalMemory(depthImage, MemoryPool::IMAGE_RGBA8_BUFFER);
	depthImageView = createImageView(device, depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
	transitionImageLayout(depthImage, depthFormat, 
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	);
}

#pragma region Memory


void padAlignment(size_t &allocOffset, VkMemoryRequirements &memReq)
{
	if ((allocOffset % memReq.alignment) != 0)
		allocOffset += memReq.alignment - (allocOffset % memReq.alignment);
}
void padAlignment(size_t &allocOffset, size_t alignment)
{
	if ((allocOffset % alignment) != 0)
		allocOffset += alignment - (allocOffset % alignment);
}
size_t VulkanRenderer::bindPhysicalMemory(VkBuffer buffer, MemoryPool pool)
{
	// Adjust the memory offset to achieve proper alignment
	VkMemoryRequirements memReq;
	vkGetBufferMemoryRequirements(device, buffer, &memReq);

	size_t freeOffset = memPool[pool].freeOffset;
	padAlignment(freeOffset, memReq);

	VkResult result = vkBindBufferMemory(device, buffer, memPool[pool].handle, freeOffset);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to bind buffer to memory.");

	// Update offset
	memPool[pool].freeOffset = freeOffset + memReq.size;
	return freeOffset;
}
size_t VulkanRenderer::bindPhysicalMemory(VkImage img, MemoryPool pool)
{
	// Adjust the memory offset to achieve proper alignment
	VkMemoryRequirements memReq;
	vkGetImageMemoryRequirements(device, img, &memReq);

	size_t freeOffset = memPool[pool].freeOffset;
	padAlignment(freeOffset, memReq);

	VkResult result = vkBindImageMemory(device, img, memPool[pool].handle, freeOffset);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to bind buffer to memory.");

	// Update offset
	memPool[pool].freeOffset = freeOffset + memReq.size;
	return freeOffset;
}

VkDescriptorSet VulkanRenderer::generateDescriptor(VkDescriptorType type, uint32_t set_binding)
{
	return createDescriptorSet(device, descriptorPools[type], &descriptorLayouts[set_binding]);
}

VkDescriptorSet VulkanRenderer::generateDescriptor(VkDescriptorType type, VkDescriptorSetLayout *layout)
{
	return createDescriptorSet(device, descriptorPools[type], layout);
}



void VulkanRenderer::transferBufferData(VkBuffer buffer, const void* data, size_t size, size_t offset)
{
	uint32_t src_offset = updateStagingBuffer(data, size);

	// Record the copying command
	VkBufferCopy bufferCopyRegion = {};
	bufferCopyRegion.srcOffset = src_offset;
	bufferCopyRegion.dstOffset = offset;
	bufferCopyRegion.size = size;

	vkCmdCopyBuffer(_transferCmd[getTransferIndex()], stagingBuffer, buffer, 1, &bufferCopyRegion);
	
}

void VulkanRenderer::transferBufferInitial(VkBuffer buffer, const void* data, size_t size, size_t offset)
{
	stagingCycleOffset = 0;
	updateStagingBuffer(data, size);

	VkCommandBuffer cmdBuffer = beginSingleCommand(device, queues[QueueType::MEM].pool);

	// Record the copying command
	VkBufferCopy bufferCopyRegion = {};
	bufferCopyRegion.srcOffset = 0;
	bufferCopyRegion.dstOffset = offset;
	bufferCopyRegion.size = size;

	vkCmdCopyBuffer(cmdBuffer, stagingBuffer, buffer, 1, &bufferCopyRegion);
	endSingleCommand_Wait(device, queues[QueueType::MEM].queue, queues[QueueType::MEM].pool, cmdBuffer);
	stagingCycleOffset = 0;
}

void VulkanRenderer::transferImageData(VkImage image, const void* data, glm::uvec3 img_size, uint32_t pixel_bytes, glm::ivec3 offset)
{
	// Currently no img transfer in loop
	vkQueueWaitIdle(queues[QueueType::MEM].queue);
	stagingCycleOffset = 0;

	uint32_t size = img_size.x * img_size.y * img_size.z * pixel_bytes;
	updateStagingBuffer(data, size);

	VkCommandBuffer cmdBuffer = beginSingleCommand(device, queues[QueueType::MEM].pool);

	// Record the copying command
	VkBufferImageCopy region = {};
	region.bufferOffset = 0;		// Initial padding
	region.bufferRowLength = 0;		// Row padding
	region.bufferImageHeight = 0;	// Column padding

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { offset.x, offset.y, offset.z };
	region.imageExtent = { img_size.x, img_size.y, img_size.z };

	vkCmdCopyBufferToImage(
		cmdBuffer,
		stagingBuffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);

	endSingleCommand_Wait(device, queues[QueueType::MEM].queue, queues[QueueType::MEM].pool, cmdBuffer);
	stagingCycleOffset = 0;
}


void VulkanRenderer::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	/* Image layout transition must be performed on the graphics queue!
	*/
	VkCommandBuffer cmdBuffer = beginSingleCommand(device, queues[QueueType::GRAPHIC].pool);
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = image;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (hasStencilComponent(format)) {
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL) {
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;	// Read & write to image.

		sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;	// Src. stage was dependent during the output stage of the hardware pipe.
		destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;		// Memory access req. synchronization during compute execution.
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	//Command
	vkCmdPipelineBarrier(
		cmdBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	endSingleCommand_Wait(device, queues[QueueType::GRAPHIC].queue, queues[QueueType::GRAPHIC].pool, cmdBuffer);
}

uint32_t VulkanRenderer::updateStagingBuffer(const void* data, size_t size)
{
	if (size > STORAGE_SIZE[MemoryPool::STAGING_BUFFER])
		throw std::runtime_error("The data requested does not fit in the staging buffer.");
	
	size_t offset = stagingCycleOffset;
	padAlignment(offset, (size_t)deviceProperties.limits.minMemoryMapAlignment);
	// Cycle staging buffer
	if (offset + std::max(deviceProperties.limits.minMemoryMapAlignment, size) > STORAGE_SIZE[MemoryPool::STAGING_BUFFER])
	{
		stagingCycleOffset = 0;
		offset = 0;
	}
	stagingCycleOffset = (uint32_t)(offset + size);

	void* bufferContents = nullptr;
	VkResult result = vkMapMemory(device, memPool[MemoryPool::STAGING_BUFFER].handle, offset, size, 0, &bufferContents);
	
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to map staging buffer to memory.");

	memcpy(bufferContents, data, size);

	vkUnmapMemory(device, memPool[MemoryPool::STAGING_BUFFER].handle);
	return (uint32_t)offset;
}

void VulkanRenderer::createStagingBuffer()
{
	stagingBuffer = createBuffer(device, STORAGE_SIZE[MemoryPool::STAGING_BUFFER], VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	memPool[MemoryPool::STAGING_BUFFER].handle = allocPhysicalMemory(device, physicalDevice, stagingBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true);
}

void VulkanRenderer::allocateBufferMemory(MemoryPool type, size_t size, VkFlags usage)
{
	VkBuffer dummy = createBuffer(device, size, usage);
	memPool[type].handle = allocPhysicalMemory(device, physicalDevice, dummy, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vkDestroyBuffer(device, dummy, nullptr);
}
void VulkanRenderer::allocateImageMemory(MemoryPool type, size_t size, VkFormat imgFormat)
{
	// Might be a bad way to set size...
	uint32_t size_dim = (uint32_t)sqrt(size);
	VkImage dummy = createTexture2D(device, size_dim, size_dim, imgFormat);
	memPool[type].handle = allocPhysicalMemory(device, physicalDevice, dummy, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vkDestroyImage(device, dummy, nullptr);
}
void VulkanRenderer::allocateImageMemory(MemoryPool type, VkImage &image, VkFormat imgFormat)
{
	memPool[type].handle = allocPhysicalMemory(device, physicalDevice, image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true);
}

#pragma endregion

#pragma region Get & Set Stuff


VkRenderPass VulkanRenderer::getFramePass()
{
	return frameBufferPass;
}

VkPipelineLayout VulkanRenderer::getFramePassLayout()
{
	return pipelineLayout;
}

VkDevice VulkanRenderer::getDevice()
{
	return device;
}

VkPhysicalDevice VulkanRenderer::getPhysical()
{
	return physicalDevice;
}
VkViewport VulkanRenderer::getViewport()
{
	return viewport;
}

VkSurfaceFormatKHR VulkanRenderer::getSwapchainFormat()
{
	return swapchainFormat;
}

VkImageView VulkanRenderer::getSwapChainView(uint32_t index)
{
	return swapchainImageViews[index];
}
VkImage VulkanRenderer::getSwapChainImg(uint32_t index)
{
	return swapchainImages[index];
}
unsigned int VulkanRenderer::getWidth()
{
	return swapchainExtent.width;
}

unsigned int VulkanRenderer::getHeight()
{
	return swapchainExtent.height;
}

void VulkanRenderer::setWinTitle(const char* title)
{
	SDL_SetWindowTitle(window, title);
}
void VulkanRenderer::setClearColor(float r, float g, float b, float a)
{
	clearColor = glm::vec4{ r, g, b, a };
}
#pragma endregion
