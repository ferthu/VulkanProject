#include <vector>

#include "Vulkan\VulkanRenderer.h"
#include "Vulkan\MaterialVulkan.h"
#include "Vulkan\MeshVulkan.h"
#include "Vulkan\VertexBufferVulkan.h"
#include "Vulkan\Texture2DVulkan.h"
#include "Vulkan\Sampler2DVulkan.h"
#include "Vulkan\RenderStateVulkan.h"
#include "Vulkan\ConstantBufferVulkan.h"
#include "Technique.h"
#include "TechniqueVulkan.h"
#include <SDL_syswm.h>
#include <assert.h>
#include <iostream>

#define VULKAN_DEVICE_IMPLEMENTATION
#include "VulkanConstruct.h"



VulkanRenderer::VulkanRenderer()
 : memPool((int)MemoryPool::Count)
{
}
VulkanRenderer::~VulkanRenderer() { }

Material* VulkanRenderer::makeMaterial(const std::string& name)
{
	MaterialVulkan* m = new MaterialVulkan(name, this);
	return (Material*)m;
}
Mesh* VulkanRenderer::makeMesh()
{
	return (Mesh*) new MeshVulkan();
}
VertexBuffer* VulkanRenderer::makeVertexBuffer(size_t size, VertexBuffer::DATA_USAGE usage)
{
	return (VertexBuffer*) new VertexBufferVulkan(this, size, usage);
}
Texture2D* VulkanRenderer::makeTexture2D()
{
	return (Texture2D*) new Texture2DVulkan(this);
}
Sampler2D* VulkanRenderer::makeSampler2D()
{
	return (Sampler2D*) new Sampler2DVulkan(this);
}
RenderState* VulkanRenderer::makeRenderState()
{
	RenderStateVulkan* newRS = new RenderStateVulkan();
	newRS->setGlobalWireFrame(&this->globalWireframeMode);
	newRS->setWireFrame(false);
	return (RenderState*)newRS;
}
std::string VulkanRenderer::getShaderPath()
{
	return "..\\assets\\Vulkan\\";
}
std::string VulkanRenderer::getShaderExtension()
{
	return ".glsl";
}
ConstantBuffer* VulkanRenderer::makeConstantBuffer(std::string NAME, unsigned int location)
{
	ConstantBufferVulkan* cb = new ConstantBufferVulkan(NAME, location);
	cb->init(this);
	return (ConstantBuffer*) cb;
}
Technique* VulkanRenderer::makeTechnique(Material* m, RenderState* r)
{
	return (Technique*) new TechniqueVulkan(m, r, this, colorPass);
}


int VulkanRenderer::initialize(unsigned int width, unsigned int height)
{
	swapchainExtent.height = height;
	swapchainExtent.width = width;

	/* Create Vulkan instance
	*/

	VkApplicationInfo applicationInfo = {};
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.pNext = nullptr;
	applicationInfo.pApplicationName = "Basic Vulkan Renderer";
	applicationInfo.applicationVersion = 1;
	applicationInfo.pEngineName = "";
	applicationInfo.apiVersion = 0;		// setting to 1 causes error when creating instance

	char* enabledLayers[] = { "VK_LAYER_LUNARG_standard_validation" };
	std::vector<char*> availableLayers = checkValidationLayerSupport(enabledLayers, sizeof(enabledLayers) / sizeof(char*));

	char* enabledExtensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface", "VK_EXT_debug_report" };
	VkInstanceCreateInfo instanceCreateInfo = {};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pNext = nullptr;
	instanceCreateInfo.flags = 0;
	instanceCreateInfo.pApplicationInfo = &applicationInfo;
	instanceCreateInfo.enabledLayerCount = (uint32_t)availableLayers.size();
	instanceCreateInfo.ppEnabledLayerNames = availableLayers.data();
	instanceCreateInfo.enabledExtensionCount = 3;
	instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions;

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
	assert(SDL_GetWindowWMInfo(window, &info) == SDL_TRUE);

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

	// Find a suitable queue family
	VkQueueFlags prefQueueFlag[] =
	{
		(VK_QUEUE_TRANSFER_BIT | VK_QUEUE_GRAPHICS_BIT)
	};
	chosenQueueFamily = pickQueueFamily(physicalDevice, prefQueueFlag, sizeof(prefQueueFlag) / sizeof(VkQueueFlags));

	// Info on queues
	VkDeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.pNext = nullptr;
	queueCreateInfo.flags = 0;
	queueCreateInfo.queueFamilyIndex = chosenQueueFamily;
	queueCreateInfo.queueCount = 1;
	float prios[] = { 1.0f };
	queueCreateInfo.pQueuePriorities = prios;

	// Info on device
	char* deviceLayers[] = { "VK_LAYER_LUNARG_standard_validation" };
	char* deviceExtensions[] = { "VK_KHR_swapchain" };

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pNext = nullptr;
	deviceCreateInfo.flags = 0;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;

	deviceCreateInfo.enabledLayerCount = 1;
	deviceCreateInfo.ppEnabledLayerNames = deviceLayers;

	deviceCreateInfo.enabledExtensionCount = 1;
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

	// Features
	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.fillModeNonSolid = true;
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

	// Create (vulkan) device
	vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);

	vkGetDeviceQueue(device, chosenQueueFamily, 0, &queue);

	// Allocate device memory
	createStagingBuffer();
	
#ifdef _DEBUG
	checkValidImageFormats(physicalDevice);
#endif
	allocateImageMemory(MemoryPool::IMAGE_RGBA8_BUFFER, STORAGE_SIZE[MemoryPool::IMAGE_RGBA8_BUFFER], VK_FORMAT_R8G8B8A8_UNORM);

	allocateBufferMemory(MemoryPool::UNIFORM_BUFFER, STORAGE_SIZE[MemoryPool::UNIFORM_BUFFER], VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	allocateBufferMemory(MemoryPool::VERTEX_BUFFER, STORAGE_SIZE[MemoryPool::VERTEX_BUFFER], VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	allocateBufferMemory(MemoryPool::INDEX_BUFFER, STORAGE_SIZE[MemoryPool::INDEX_BUFFER], VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

	/* Create swap chain
	*/

	// Check that queue supports presenting
	VkBool32 presentingSupported;
	vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, chosenQueueFamily, windowSurface, &presentingSupported);

	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, windowSurface, &surfaceCapabilities);

	if (presentingSupported == VK_FALSE)
		throw std::runtime_error("The selected queue does not support presenting. Do more programming >:|");

	// Get supported formats
	std::vector<VkSurfaceFormatKHR> formats;
	ALLOC_QUERY_ASSERT(result, vkGetPhysicalDeviceSurfaceFormatsKHR, formats, physicalDevice, windowSurface);
	swapchainFormat = formats[0];

	// Choose the mode for the swap chain that determines how the frame buffers are swapped.
	VkPresentModeKHR presentModePref[] =
	{
#ifdef NO_VSYNC
		VK_PRESENT_MODE_IMMEDIATE_KHR,// Immediately present images to screen
#endif
		VK_PRESENT_MODE_MAILBOX_KHR // Oldest finished frame are replaced if 'framebuffer' queue is filled.
	};
	VkPresentModeKHR presentMode = chooseSwapPresentMode(physicalDevice, windowSurface, presentModePref, sizeof(presentModePref) / sizeof(VkPresentModeKHR));

	// Create swap chain
	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.pNext = nullptr;
	swapchainCreateInfo.flags = 0;
	swapchainCreateInfo.surface = windowSurface;
	swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount;
	swapchainCreateInfo.imageFormat = formats[0].format;	// Just select the first available format
	swapchainCreateInfo.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	swapchainCreateInfo.imageExtent = swapchainExtent;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
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

	// Create render pass
	colorPass = createRenderPass_SingleColor(device, swapchainCreateInfo.imageFormat);

	// Create frame buffers.
	const int NUM_FRAME_ATTACH = 1;
	swapChainFramebuffers.resize(swapchainImages.size() / NUM_FRAME_ATTACH);
	for (size_t i = 0; i < swapChainFramebuffers.size(); i++)
	{
		VkImageView attachments[] = {
			swapchainImageViews[i]
		};
		swapChainFramebuffers[i] = createFramebuffer(device, colorPass, swapchainExtent, attachments, NUM_FRAME_ATTACH);
	}


	// Create staging command pool
	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.pNext = nullptr;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	commandPoolCreateInfo.queueFamilyIndex = chosenQueueFamily;
	result = vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &stagingCommandPool);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create staging command pool.");

	// Create drawing command pool
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	result = vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &drawingCommandPool);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create staging command pool.");


	// Stuff
	imageAvailableSemaphore = createSemaphore(device);
	renderFinishedSemaphore = createSemaphore(device);

	for (uint32_t i = 0; i < MAX_DESCRIPTOR_POOLS; i++)
		descriptorPools[i] = NULL;
	descriptorPools[VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER]
		= createDescriptorPoolSingle(device, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2000);
	descriptorPools[VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER]
		= createDescriptorPoolSingle(device, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2000);

	return 0;
}

void VulkanRenderer::setWinTitle(const char* title)
{
	SDL_SetWindowTitle(window, title);
}
void VulkanRenderer::present()
{

}
int VulkanRenderer::shutdown()
{
	// Wait for device to finish before shuting down..
	vkDeviceWaitIdle(device);

	// Clean up Vulkan
	for (unsigned int i = 0; i < MAX_DESCRIPTOR_POOLS; i++)
	{
		if(descriptorPools[i])
			vkDestroyDescriptorPool(device, descriptorPools[i], nullptr);
	}
	vkDestroyCommandPool(device, drawingCommandPool, nullptr);
	vkDestroyCommandPool(device, stagingCommandPool, nullptr);
	vkDestroyBuffer(device, stagingBuffer, nullptr);

	vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
	vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);

	for (auto framebuffer : swapChainFramebuffers) 
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	for (int i = 0; i < swapchainImageViews.size(); ++i)
		vkDestroyImageView(device, swapchainImageViews[i], nullptr);
	for(uint32_t i = 0; i < memPool.size(); i++)
		vkFreeMemory(device, memPool[i].handle, nullptr);

	vkDestroySwapchainKHR(device, swapchain, nullptr);
	vkDestroyRenderPass(device, colorPass, nullptr);
	vkDestroySurfaceKHR(instance, windowSurface, nullptr);
	vkDestroyDevice(device, nullptr);
	vkDestroyInstance(instance, nullptr);

	//Clean up SDL
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;	// temp
}

void VulkanRenderer::setClearColor(float r, float g, float b, float a)
{
	clearColor = glm::vec4{ r, g, b, a };
}
void VulkanRenderer::clearBuffer(unsigned int flag)
{

}
void VulkanRenderer::setRenderState(RenderState* ps)
{

}
void VulkanRenderer::submit(Mesh* mesh)
{
	drawLists[mesh->technique].push_back(mesh);
}
void VulkanRenderer::frame()
{
	uint32_t imageIndex;
	vkAcquireNextImageKHR(device, swapchain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = drawingCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(device, &allocInfo, &_frameCmdBuf) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
	beginInfo.pInheritanceInfo = nullptr; // Optional
	vkBeginCommandBuffer(_frameCmdBuf, &beginInfo);

	//Render pass
	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = colorPass;
	renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = swapchainExtent;
	VkClearValue clearColor = { this->clearColor.r, this->clearColor.g, this->clearColor.b, this->clearColor.a };
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(_frameCmdBuf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	// Draw stuff..?:)
	
	for (auto work : drawLists)
	{
		TechniqueVulkan *vk_tec = (TechniqueVulkan*)work.first;
		assert(dynamic_cast<TechniqueVulkan*>(work.first));

		vkCmdBindPipeline(_frameCmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_tec->pipeline);
		work.first->enable(this);
		for (auto mesh : work.second)
		{
			uint32_t numberElements = (uint32_t)mesh->geometryBuffers[0].numElements;
			for (auto t : mesh->textures)
			{
				// we do not really know here if the sampler has been
				// defined in the shader.
				t.second->bind(t.first, mesh->technique->getMaterial());
			}
			for (auto element : mesh->geometryBuffers) {
				mesh->bindIAVertexBuffer(element.first);
			}
			mesh->txBuffer->bind(vk_tec->getMaterial());
			vkCmdDraw(_frameCmdBuf, numberElements, 1, 0, 0);
		}
	}
	drawLists.clear();

	vkCmdEndRenderPass(_frameCmdBuf);
	if (vkEndCommandBuffer(_frameCmdBuf) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}

	// Submit

	VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
	VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &_frameCmdBuf;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;
	if (vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr; // Optional
	vkQueuePresentKHR(queue, &presentInfo);

	vkQueueWaitIdle(queue);
	vkResetCommandPool(device, drawingCommandPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
}


VkCommandBuffer VulkanRenderer::getFrameCmdBuf()
{
	return _frameCmdBuf;
}

VkDevice VulkanRenderer::getDevice()
{
	return device;
}

VkPhysicalDevice VulkanRenderer::getPhysical()
{
	return physicalDevice;
}


void padAlignment(size_t &allocOffset, VkMemoryRequirements &memReq)
{
	if ((allocOffset % memReq.alignment) != 0)
		allocOffset += memReq.alignment - (allocOffset % memReq.alignment);
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

VkDescriptorSet VulkanRenderer::generateDescriptor(VkDescriptorType type, VkDescriptorSetLayout &layout)
{
	return createDescriptorSet(device, descriptorPools[type], &layout);
}


void VulkanRenderer::transferBufferData(VkBuffer buffer, const void* data, size_t size, size_t offset)
{
	updateStagingBuffer(data, size);

	VkCommandBuffer cmdBuffer = beginSingleCommand(device, stagingCommandPool);

	// Record the copying command
	VkBufferCopy bufferCopyRegion = {};
	bufferCopyRegion.srcOffset = 0;
	bufferCopyRegion.dstOffset = offset;
	bufferCopyRegion.size = size;

	vkCmdCopyBuffer(cmdBuffer, stagingBuffer, buffer, 1, &bufferCopyRegion);
	
	endSingleCommand_Wait(device, queue, stagingCommandPool, cmdBuffer);
}

void VulkanRenderer::transferImageData(VkImage image, const void* data, glm::uvec3 img_size, uint32_t pixel_bytes, glm::ivec3 offset)
{
	uint32_t size = img_size.x * img_size.y * img_size.z * pixel_bytes;
	updateStagingBuffer(data, size);

	VkCommandBuffer cmdBuffer = beginSingleCommand(device, stagingCommandPool);

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

	endSingleCommand_Wait(device, queue, stagingCommandPool, cmdBuffer);
}


void VulkanRenderer::transitionImageFormat(VkImage image, VkFormat format, VkImageLayout fromLayout, VkImageLayout toLayout)
{
	VkCommandBuffer cmdBuffer = beginSingleCommand(device, stagingCommandPool);



	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = fromLayout;
	barrier.newLayout = toLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;
	if (fromLayout == VK_IMAGE_LAYOUT_UNDEFINED && toLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (fromLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && toLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
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

	endSingleCommand_Wait(device, queue, stagingCommandPool, cmdBuffer);
}

VkSurfaceFormatKHR VulkanRenderer::getSwapchainFormat()
{
	return swapchainFormat;
}

unsigned int VulkanRenderer::getWidth()
{
	return swapchainExtent.width;
}

unsigned int VulkanRenderer::getHeight()
{
	return swapchainExtent.height;
}

void VulkanRenderer::updateStagingBuffer(const void* data, size_t size)
{
	if (size > STORAGE_SIZE[MemoryPool::STAGING_BUFFER])
		throw std::runtime_error("The data requested does not fit in the staging buffer.");

	void* bufferContents = nullptr;
	VkResult result = vkMapMemory(device, memPool[MemoryPool::STAGING_BUFFER].handle, 0, VK_WHOLE_SIZE, 0, &bufferContents);

	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to map staging buffer to memory.");

	memcpy(bufferContents, data, size);

	vkUnmapMemory(device, memPool[MemoryPool::STAGING_BUFFER].handle);
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
