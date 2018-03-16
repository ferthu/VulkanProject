#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan\vulkan.h"

#include "SDL/SDL.h"
#include "glm/glm.hpp"
#include <memory>

#pragma comment(lib, "vulkan-1.lib")
#pragma comment(lib,"SDL2.lib")
#pragma comment(lib,"SDL2main.lib")
#include "VulkanConstruct.h"

#include "VertexBufferVulkan.h"
#include "ShaderVulkan.h"
#include "TechniqueVulkan.h"

/* Remember!!! number of device allocations is limited (very).
*/
struct DevMemoryAllocation
{
	VkDeviceMemory handle;	// The device memory handle
	size_t freeOffset;	// Offset to the next free area in the storage memory

	DevMemoryAllocation() : handle(NULL), freeOffset(0) {}
};
enum MemoryPool
{
	STAGING_BUFFER = 0,		// GPU memory allocation accessible to CPU. Used to move data from CPU to GPU
	UNIFORM_BUFFER = 1,		// GPU memory allocation used to store uniform data.
	IMAGE_RGBA8_BUFFER = 2,	// GPU memory allocation to store image data.
	VERTEX_BUFFER = 3,
	INDEX_BUFFER = 4,
	IMAGE_D16_BUFFER = 5,
	Count = 6
};

enum RenderFlagBits
{
	TRIPLE_BUFFERED = 0x00000001
};

const int MAX_DESCRIPTOR_POOLS = 12;
// Size in bytes of the memory types used
const uint32_t STORAGE_SIZE[(int)MemoryPool::Count] = { 2048 * 2048*64, 2048 * 2048*64, 2048 * 2048, 2048 * 2048 * 64, 1024 * 1024 * 64, 1024 * 1024 * 10 };


class Scene;

enum QueueType {
	MEM = 0,
	GRAPHIC = 1,
	COMPUTE = 2,
	COMPUTE2 = 3,
	COUNT = 4
};
// Set constant values for now

class VulkanRenderer
{
public:

	VulkanRenderer();
	~VulkanRenderer();

	void setWinTitle(const char* title);

	int initialize(Scene *scene, unsigned int width, unsigned int height, uint32_t BIT_FLAGS);
	void frame();
	void present();

	struct FrameInfo
	{
		VkCommandBuffer _buf;
		uint32_t _swapChainIndex;
		VkImage _swapChainImage;
	};
	// Begins a frame pass with a supplied custom frame buffer
	FrameInfo beginFramePass(VkFramebuffer* frameBuffer = NULL);

	// These two functions are used instead of beginFramePass when their functionality needs to be separated
	FrameInfo beginCommandBuffer();
	void beginRenderPass(VkCommandBuffer cmdBuf, VkFramebuffer* frameBuffer = NULL);
	void endRenderPass();

	FrameInfo beginCompute(uint32_t computeQueueIndex = 0);
	void submitFramePass();
	void submitCompute(uint32_t computeQueueIndex = 0, bool syncPrevious = true);

	virtual int beginShutdown();
	int shutdown();

	void setClearColor(float r, float g, float b, float a);
	void clearBuffer(unsigned int flag);

	VkDevice getDevice();
	VkPhysicalDevice getPhysical();

	const VkViewport& getViewport();

	/* Bind a physical memory partition on the device to the buffer from the specific memory pool. */
	size_t bindPhysicalMemory(VkBuffer buffer, MemoryPool memPool);
	size_t bindPhysicalMemory(VkImage img, MemoryPool pool);

	VkDescriptorSet generateDescriptor(VkDescriptorType type, VkDescriptorSetLayout *layout);
	VkDescriptorSet generateDescriptor(VkDescriptorType type, uint32_t set_binding);

	/* Transfer data to the specific buffer. */
	void transferBufferData(VkBuffer buffer, const void* data, size_t byteSize, size_t offset);
	void transferBufferInitial(VkBuffer buffer, const void* data, size_t byteSize, size_t offset);
	void transferImageData(VkImage image, const void* data, glm::uvec3 img_size, uint32_t pixel_bytes, glm::ivec3 offset = glm::ivec3(0));
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout fromLayout, VkImageLayout toLayout);

	/* Acquire the RenderPass for the final frame buffers presented to the screen.
	*/
	VkRenderPass getFramePass();
	VkPipelineLayout getFramePassLayout();
	int getQueueFamily(QueueType queue) { return queues[queue].family; };
	uint32_t getFrameIndex() { return frameCycle; }
	uint32_t getTransferIndex() { return !frameCycle; }
	size_t getSwapChainLength() { return swapchainImages.size(); }

	VkSurfaceFormatKHR getSwapchainFormat();
	VkImageView getSwapChainView(uint32_t index);
	VkImage getSwapChainImg(uint32_t index);

	unsigned int getWidth();
	unsigned int getHeight();

	VkDescriptorSetLayout getDescriptorSetLayout(uint32_t index);

private:
	Scene * scene;

	VkInstance instance;
	VkDevice device;
	int chosenPhysicalDevice;
	VkPhysicalDevice physicalDevice;
	VkPhysicalDeviceProperties deviceProperties;
	std::vector<DevMemoryAllocation> memPool;// Memory pool of device memory. Remember!!! number of device allocations is limited (very).

	bool globalWireframeMode = false;

	SDL_Window* window;
	VkSurfaceKHR windowSurface;
	VkSwapchainKHR swapchain;

	std::vector<VkImage> swapchainImages;				// Array of images in the swapchain, use vkAquireNextImageKHR(...) to aquire image for drawing to
	std::vector<VkImageView> swapchainImageViews;		// Image views for the swap chain images
	std::vector<VkFramebuffer> swapChainFramebuffers;	// Combined sets of images that make up each frame buffer.

	VkFormat depthFormat;								// Depth image format.
	VkImage depthImage;									// Frame buffer depth image
	VkImageView depthImageView;							// Frame buffer depth image view.
	glm::vec4 clearColor;								// Frame buffer clear color.

	/* Render pass & Pipelines
	Dependent constructs.
	Render pass determines attached framebuffers ...
	Pipelines depend on a render pass and when set determine renderstate, shaders, culling...
	Pipeline layouts however can be shared between any combination of setups aslong there are no conflicts in the bindings
	(they can include redundant binding slots..)
	*/
	VkRenderPass frameBufferPass;
	VkPipelineLayout pipelineLayout;
	std::vector<VkDescriptorSetLayout> descriptorLayouts;

	VkViewport viewport;

	VkSemaphore imageAvailable;
	VkSemaphore renderFinished[2], computeFinished[2];		// Multiple render signals as it could be useful to overlap frame buffers.
	VkFence renderFence[2], computeFence[2];
	uint32_t waitQueueLen;
	VkSemaphore waitQueue[QueueType::COUNT];

	VkCommandBuffer _frameCmdBuf[2], _computeCmdBuf[2];
	VkCommandBuffer _transferCmd[2];
	VkFence			_transferFences[2];
	uint32_t swapChainImgIndex;								// Tracks frame buffer index for current frame
	uint32_t frameCycle = 0, stagingCycleOffset = 0;	// Tracks transfer cycle
	bool firstFrame = 1;
	/*
	*/
	VkDescriptorPool descriptorPools[MAX_DESCRIPTOR_POOLS];
	
	VkBuffer stagingBuffer;			// Buffer to temporarily hold data being transferred to GPU

	vk::QueueConstruct queues;

	VkSurfaceFormatKHR swapchainFormat;
	VkExtent2D swapchainExtent;

	// Number of attatchments in the frame buffers
	uint32_t NUM_FRAME_ATTACH = 0;

	void createStagingBuffer();
	uint32_t updateStagingBuffer(const void* data, size_t size);								// Writes memory from data into the staging buffer
	void allocateBufferMemory(MemoryPool type, size_t size, VkFlags usage);					// Allocates memory pool buffer data
	void allocateImageMemory(MemoryPool type, size_t size, VkFormat imgFormat);				// Allocates memory pool image data
	void allocateImageMemory(MemoryPool type, VkImage &image, VkFormat imgFormat);

	void nextFrame();

	void createDepthComponents();
};