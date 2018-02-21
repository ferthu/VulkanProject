#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan\vulkan.h"

#include "SDL/SDL.h"
#include "glm/glm.hpp"

#pragma comment(lib, "vulkan-1.lib")
#pragma comment(lib,"glew32.lib")
#pragma comment(lib,"SDL2.lib")
#pragma comment(lib,"SDL2main.lib")
#include "VulkanConstruct.h"

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
	Count = 5
};

const int MAX_DESCRIPTOR_POOLS = 12;

// Size in bytes of the memory types used
const uint32_t STORAGE_SIZE[(int)MemoryPool::Count] = { 2048 * 2048, 1024*1024, 1024 * 1024, 1024 * 1024, 1024 * 1024 };

class VulkanRenderer
{
public:

	VulkanRenderer();
	~VulkanRenderer();

	Material* makeMaterial(const std::string& name);
	Mesh* makeMesh();
	VertexBuffer* makeVertexBuffer(size_t size, VertexBuffer::DATA_USAGE usage);
	Texture2D* makeTexture2D();
	Sampler2D* makeSampler2D();
	RenderState* makeRenderState();
	std::string getShaderPath();
	std::string getShaderExtension();
	ConstantBuffer* makeConstantBuffer(std::string NAME, unsigned int location);
	Technique* makeTechnique(Material* m, RenderState* r);


	int initialize(unsigned int width = 640, unsigned int height = 480);
	void setWinTitle(const char* title);
	void present();

	virtual int beginShutdown();
	int shutdown();

	void setClearColor(float r, float g, float b, float a);
	void clearBuffer(unsigned int flag);
	void frame();

	VkDevice getDevice();
	VkPhysicalDevice getPhysical();

	/* Bind a physical memory partition on the device to the buffer from the specific memory pool. */
	size_t bindPhysicalMemory(VkBuffer buffer, MemoryPool memPool);
	size_t bindPhysicalMemory(VkImage img, MemoryPool pool);

	VkDescriptorSet generateDescriptor(VkDescriptorType type, uint32_t set_binding);

	/* Transfer data to the specific buffer. */
	void transferBufferData(VkBuffer buffer, const void* data, size_t size, size_t offset);
	void transferBufferInitial(VkBuffer buffer, const void* data, size_t size, size_t offset);
	void transferImageData(VkImage image, const void* data, glm::uvec3 img_size, uint32_t pixel_bytes, glm::ivec3 offset = glm::ivec3(0));
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout fromLayout, VkImageLayout toLayout);

	VkSurfaceFormatKHR getSwapchainFormat();
	VkCommandBuffer getFrameCmdBuf();
	uint32_t getFrameIndex() { return frameCycle; }
	uint32_t getTransferIndex() { return !frameCycle; }
	VkPipelineLayout getPipelineLayout();


	unsigned int getWidth();
	unsigned int getHeight();

private:

	VkInstance instance;
	VkDevice device;
	int chosenPhysicalDevice;
	VkPhysicalDevice physicalDevice;
	VkPhysicalDeviceProperties deviceProperties;
	std::vector<DevMemoryAllocation> memPool;// Memory pool of device memory

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
	VkRenderPass colorPass;
	VkPipelineLayout pipelineLayout;
	std::vector<VkDescriptorSetLayout> descriptorLayouts;

	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;
	VkCommandBuffer _frameCmdBuf;
	VkCommandBuffer _transferCmd[2];
	VkFence			_transferFences[2];
	uint32_t frameBufIndex;								// Tracks frame buffer index for current frame
	uint32_t frameCycle = 0, stagingCycleOffset = 0;	// Tracks transfer cycle
	bool firstFrame = 1;
	/*
	*/
	VkDescriptorPool descriptorPools[MAX_DESCRIPTOR_POOLS];
	
	VkBuffer stagingBuffer;			// Buffer to temporarily hold data being transferred to GPU

	enum QueueType {
		MEM = 0,
		GRAPHIC = 1,
		COUNT = 2
	};
	vk::QueueConstruct queues[QueueType::COUNT];

	int chosenQueueFamily_asd;		// The queue family to be used
	VkQueue queue_asd;		// Handle to the queue used

	VkSurfaceFormatKHR swapchainFormat;
	VkExtent2D swapchainExtent;

	void createStagingBuffer();
	uint32_t updateStagingBuffer(const void* data, size_t size);								// Writes memory from data into the staging buffer
	void allocateBufferMemory(MemoryPool type, size_t size, VkFlags usage);					// Allocates memory pool buffer data
	void allocateImageMemory(MemoryPool type, size_t size, VkFormat imgFormat);				// Allocates memory pool image data
	void allocateImageMemory(MemoryPool type, VkImage &image, VkFormat imgFormat);

	void nextFrame();

	void defineDescriptorLayout();
	void generatePipelineLayout();
	void createDepthComponents();
};