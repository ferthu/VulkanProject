#pragma once

#include "../Renderer.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan\vulkan.h"

#include <SDL.h>
#include <GL/glew.h>
#include "../glm/glm.hpp"

#pragma comment(lib, "vulkan-1.lib")
#pragma comment(lib,"glew32.lib")
#pragma comment(lib,"SDL2.lib")
#pragma comment(lib,"SDL2main.lib")

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

class VulkanRenderer : public Renderer
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
	int shutdown();

	void setClearColor(float r, float g, float b, float a);
	void clearBuffer(unsigned int flag);
	void setRenderState(RenderState* ps);
	void submit(Mesh* mesh);	// Get translation buffer from mesh, send it to technique, which binds it and then binds the descriptor set to the pipeline
	void frame();

	VkDevice getDevice();
	VkPhysicalDevice getPhysical();

	/* Bind a physical memory partition on the device to the buffer from the specific memory pool. */
	size_t bindPhysicalMemory(VkBuffer buffer, MemoryPool memPool);
	size_t bindPhysicalMemory(VkImage img, MemoryPool pool);

	VkDescriptorSet generateDescriptor(VkDescriptorType type, VkDescriptorSetLayout &layout);

	/* Transfer data to the specific buffer. */
	void transferBufferData(VkBuffer buffer, const void* data, size_t size, size_t offset);
	void transferImageData(VkImage image, const void* data, glm::uvec3 img_size, uint32_t pixel_bytes, glm::ivec3 offset = glm::ivec3(0));
	void transitionImageFormat(VkImage image, VkFormat format, VkImageLayout fromLayout, VkImageLayout toLayout);

	VkSurfaceFormatKHR getSwapchainFormat();
	VkCommandBuffer getFrameCmdBuf();


	unsigned int getWidth();
	unsigned int getHeight();

private:

	VkInstance instance;
	VkDevice device;
	int chosenPhysicalDevice;
	VkPhysicalDevice physicalDevice;
	std::vector<DevMemoryAllocation> memPool;// Memory pool of device memory

	bool globalWireframeMode = false;

	SDL_Window* window;
	VkSurfaceKHR windowSurface;
	VkSwapchainKHR swapchain;

	std::vector<VkImage> swapchainImages;				// Array of images in the swapchain, use vkAquireNextImageKHR(...) to aquire image for drawing to
	std::vector<VkImageView> swapchainImageViews;		// Image views for the swap chain images
	std::vector<VkFramebuffer> swapChainFramebuffers;	// Combined sets of images that make up each frame buffer.
	glm::vec4 clearColor;
	VkRenderPass colorPass;

	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;
	VkCommandBuffer _frameCmdBuf;

	/*
	*/
	VkDescriptorPool descriptorPools[MAX_DESCRIPTOR_POOLS];

	std::unordered_map<Technique*, std::vector<Mesh*>> drawLists;
	

	VkBuffer stagingBuffer;			// Buffer to temporarily hold data being transferred to GPU

	VkCommandPool stagingCommandPool;	// Allocates commands used when moving data to the GPU
	VkCommandPool drawingCommandPool;	// Allocates commands used for drawing

	int chosenQueueFamily;		// The queue family to be used
	VkQueue queue;		// Handle to the queue used

	VkSurfaceFormatKHR swapchainFormat;
	VkExtent2D swapchainExtent;

	void createStagingBuffer();
	void updateStagingBuffer(const void* data, size_t size);								// Writes memory from data into the staging buffer
	void allocateBufferMemory(MemoryPool type, size_t size, VkFlags usage);					// Allocates memory pool buffer data
	void allocateImageMemory(MemoryPool type, size_t size, VkFormat imgFormat);				// Allocates memory pool image data
};