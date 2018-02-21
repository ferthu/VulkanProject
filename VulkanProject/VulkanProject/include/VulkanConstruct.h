#pragma once

#include "vulkan\vulkan.h"
#include <assert.h>
#include <algorithm>
#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>


#pragma region Inline and type defs

#define ALLOC_QUERY_NOPARAM(fn, vec) { unsigned int count=0; fn(&count, nullptr); vec.resize(count); fn(&count, vec.data()); }
#define ALLOC_QUERY(fn, vec, ...) { unsigned int count=0; fn(__VA_ARGS__, &count, nullptr); vec.resize(count); fn(__VA_ARGS__, &count, vec.data()); }
#define ALLOC_QUERY_ASSERT(result, fn, vec, ...) { unsigned int count=0; fn(__VA_ARGS__, &count, nullptr); vec.resize(count); result = fn(__VA_ARGS__, &count, vec.data()); assert(result == VK_SUCCESS); }


/* Check if a mode is available in the list*/
template<class T>
inline bool hasMode(int mode, T *mode_list, size_t list_len)
{
	for (size_t i = 0; i < list_len; i++)
	{
		if (mode_list[i] == mode)
			return true;
	}
	return false;
}
/* Check if the flags are set in the property. */
template<class T>
inline bool hasFlag(T property, uint32_t flags)
{
	return (property & flags) == flags;
}
/* Find if the flags are equal. */
template<class T>
inline bool matchFlag(T property, uint32_t flags)
{
	return property == flags;
}
/* Unset bit in the flag. */
template<class T>
inline T rmvFlag(T property, uint32_t rmv)
{
	return property & ~rmv;
}

#pragma endregion

#pragma region Structs
namespace vk
{
	struct QueueConstruct
	{
		int family;
		VkCommandPool pool;
		VkQueue queue;

		/* Destroy the queue related resources (the VkCommandPool)
		*/
		void destroyQueue(VkDevice dev);
		/* Acquire the queue from device.
		*/
		void getDeviceQueue(VkDevice dev);
		int createCommandPool(VkDevice dev, VkCommandPoolCreateFlags flag);
	};
}
#pragma endregion

/* Function declarations
*/

/*	Queue selection */
int anyQueueFamily(VkPhysicalDevice &device, VkQueueFlags* pref_queueFlag, int num_flag);
int matchQueueFamily(VkPhysicalDevice &device, VkQueueFlags* pref_queueFlag, int num_flag);
int pickQueueFamily(VkPhysicalDevice &device, VkQueueFlags* pref_queueFlag, int num_flag);

VkDeviceQueueCreateInfo defineQueues(int family, float *prio, uint32_t num_queues);
VkDeviceQueueCreateInfo defineQueue(int family, float prio);

/* Device*/


/* Device selection */
namespace vk
{
	/*	Determines if a physical device is suitable for the system. Return rank of the device, ranks greater then 0 will be available for selection. */
	typedef int(*isDeviceSuitable)(VkPhysicalDevice &device, VkPhysicalDeviceProperties &prop, VkPhysicalDeviceFeatures &feat, VkQueueFamilyProperties *queue_family_prop, size_t len_family_prop);

	/* Function selecting any dedicated device making a preference for discrete over integrated devices.
	*/
	int specifyAnyDedicatedDevice(VkPhysicalDevice &device, VkPhysicalDeviceProperties &prop, VkPhysicalDeviceFeatures &feat, VkQueueFamilyProperties *queue_family_prop, size_t len_family_prop);
}
int choosePhysicalDevice(VkInstance &instance, VkSurfaceKHR &surface, vk::isDeviceSuitable deviceSpec, VkQueueFlags queueSupportReq, VkPhysicalDevice &result);

std::vector<char*> checkValidationLayerSupport(char** validationLayers, size_t num_layer);
void checkValidImageFormats(VkPhysicalDevice device);

/* Swap chain */

VkPresentModeKHR chooseSwapPresentMode(VkPhysicalDevice &device, VkSurfaceKHR &surface, VkPresentModeKHR *prefered_modes, size_t num_prefered);
VkFramebuffer createFramebuffer(VkDevice device, VkRenderPass pass, VkExtent2D frameDim, VkImageView *attachments, uint32_t num_attachment);
VkRenderPass createRenderPass_SingleColor(VkDevice device, VkFormat swapChainImgFormat);
VkRenderPass createRenderPass_SingleColorDepth(VkDevice device, VkFormat swapChainImgFormat, VkFormat depthFormat);

VkFormat findSupportedFormat(VkPhysicalDevice physDevice, const VkFormat* candidates, size_t num_cand, VkImageTiling tiling, VkFormatFeatureFlags features);
VkFormat findDepthFormat(VkPhysicalDevice physDevice);
bool hasStencilComponent(VkFormat format);



/* Memory */

VkBuffer createBuffer(VkDevice device, size_t byte_size, VkBufferUsageFlags usage, uint32_t queueCount = 0, uint32_t *queueFamilyIndices = nullptr);
VkVertexInputBindingDescription defineVertexBinding(uint32_t bind_index, uint32_t vertex_bytes, VkVertexInputRate inputRate = VK_VERTEX_INPUT_RATE_VERTEX);
VkVertexInputAttributeDescription defineVertexAttribute(uint32_t bind_index, uint32_t loc_index, VkFormat format, uint32_t attri_offset);

VkImage createTexture2D(VkDevice device, uint32_t width, uint32_t height, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL);
VkImage createDepthBuffer(VkDevice device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL);
VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D);

VkSampler createSampler(VkDevice device, VkFilter magFilter = VK_FILTER_LINEAR, VkFilter minFilter = VK_FILTER_LINEAR, 
	VkSamplerAddressMode wrap_s = VK_SAMPLER_ADDRESS_MODE_REPEAT, VkSamplerAddressMode wrap_t = VK_SAMPLER_ADDRESS_MODE_REPEAT);

VkDeviceMemory allocPhysicalMemory(VkDevice device, VkPhysicalDevice physicalDevice, VkBuffer buffer, VkMemoryPropertyFlags properties, bool bindToBuffer = false);
VkDeviceMemory allocPhysicalMemory(VkDevice device, VkPhysicalDevice physicalDevice, VkImage image, VkMemoryPropertyFlags properties, bool bindToImage = false);
VkDeviceMemory allocPhysicalMemory(VkDevice device, VkPhysicalDevice physicalDevice, VkMemoryRequirements requirements, VkMemoryPropertyFlags properties);

/* Commands */

VkCommandBuffer allocateCmdBuf(VkDevice device, VkCommandPool commandPool);
void beginCmdBuf(VkCommandBuffer cmdBuf, VkFlags flag = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
VkCommandBuffer beginSingleCommand(VkDevice device, VkCommandPool commandPool);
void endSingleCommand(VkDevice device, VkQueue queue, VkCommandBuffer commandBuf, VkFence fence = VK_NULL_HANDLE);
void endSingleCommand_Wait(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkCommandBuffer commandBuf);
void releaseCommandBuffer(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkCommandBuffer commandBuf);

VkSemaphore createSemaphore(VkDevice device);
VkFence createFence(VkDevice device, bool signaled = false);
void waitFence(VkDevice device, VkFence fence);

#pragma region Descriptors

/* Fill a VkWriteDescriptorSet with VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER write info.
*/
void writeDescriptorStruct_IMG_COMBINED(VkWriteDescriptorSet &writeInfo, VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t dstArrayElem, uint32_t descriptorCount, VkDescriptorImageInfo *imageInfo);
/* Fill a VkWriteDescriptorSet with VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER write info.
*/
void writeDescriptorStruct_UNI_BUFFER(VkWriteDescriptorSet &writeInfo, VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t dstArrayElem, uint32_t descriptorCount, VkDescriptorBufferInfo* bufferInfo);
/* Write a image layout binding
*/
void writeLayoutBinding(VkDescriptorSetLayoutBinding &layoutBinding, uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage);
/* Create a VkDescriptorSetLayout from the bindings.
*/
VkDescriptorSetLayout createDescriptorLayout(VkDevice device, VkDescriptorSetLayoutBinding *bindings, size_t num_binding);
VkDescriptorPool createDescriptorPoolSingle(VkDevice device, VkDescriptorType type, uint32_t poolSize);
VkDescriptorPool createDescriptorPool(VkDevice device, VkDescriptorPoolSize *sizeTypes, uint32_t num_types, uint32_t poolSize);
VkDescriptorSet createDescriptorSet(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout *layouts);
#pragma endregion

#pragma region Pipeline

VkViewport defineViewport(float width, float height);
VkViewport defineViewport(float x, float y, float width, float height, float minDepth = 0.f, float maxDepth = 1.f);
VkRect2D defineScissorRect(VkViewport &viewport);
VkRect2D defineScissorRect(int32_t x, int32_t y, uint32_t width, uint32_t height);
VkPipelineViewportStateCreateInfo defineViewportState(VkViewport *viewport, VkRect2D *scissor);

VkPipelineInputAssemblyStateCreateInfo defineInputAssembly(VkPrimitiveTopology primitiveType = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VkBool32 indexLoopEnable = VK_FALSE);
VkPipelineMultisampleStateCreateInfo defineMultiSampling_OFF();
VkPipelineColorBlendStateCreateInfo defineBlendState_LogicOp(VkPipelineColorBlendAttachmentState *blendStateAttachments, uint32_t num_attachments, VkLogicOp logic_op, glm::vec4 blendConstants = glm::vec4(0));
VkPipelineColorBlendStateCreateInfo defineBlendState(VkPipelineColorBlendAttachmentState *blendStateAttachments, uint32_t num_attachments, glm::vec4 blendConstants = glm::vec4(0));
/* Define a simple uniform layout using uniform buffers (no push constants).
*/
VkPipelineLayout createPipelineLayout(VkDevice device, VkDescriptorSetLayout *descriptorSet, uint32_t num_descriptors);


typedef enum RasterizationFlagBits
{
	WIREFRAME_BIT = 0x00000001,
	DEPTH_CLAMP_BIT = 0x00000002,
	CLOCKWISE_FACE_BIT = 0x00000004,
	NO_RASTERIZATION_BIT = 0x00000008	// Primitives are discarded before rasterization stage...
} RasterizationFlagBits;
VkPipelineRasterizationStateCreateInfo defineRasterizationState(uint32_t rasterFlags, VkCullModeFlags cullModeFlags, float lineWidth = 1.f);
VkPipelineDepthStencilStateCreateInfo defineDepthState();

VkPipelineShaderStageCreateInfo defineShaderStage(VkShaderStageFlagBits stage, VkShaderModule shader, const char* entryFunc = "main");

VkPipelineVertexInputStateCreateInfo defineVertexBufferBindings(
	VkVertexInputBindingDescription *bindings, uint32_t num_buffers, 
	VkVertexInputAttributeDescription *attributes, uint32_t num_attri);

#pragma endregion

#ifdef VULKAN_DEVICE_IMPLEMENTATION
#include <iostream>

#pragma region Structs
namespace vk
{
	/* Destroy the queue related resources (the VkCommandPool)
	*/
	void QueueConstruct::destroyQueue(VkDevice dev)
	{
		vkDestroyCommandPool(dev, pool, nullptr);
	}
	/* Acquire the queue from device.
	*/
	void QueueConstruct::getDeviceQueue(VkDevice dev)
	{
		vkGetDeviceQueue(dev, family, 0, &queue);
	}
	int QueueConstruct::createCommandPool(VkDevice dev, VkCommandPoolCreateFlags flag)
	{
		VkCommandPoolCreateInfo commandPoolCreateInfo = {};
		commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCreateInfo.pNext = nullptr;
		commandPoolCreateInfo.flags = flag;
		commandPoolCreateInfo.queueFamilyIndex = family;
		VkResult err = vkCreateCommandPool(dev, &commandPoolCreateInfo, nullptr, &pool);
		if (err != VK_SUCCESS)
			return -1;
		return 0;
	}
}
#pragma endregion

#pragma region Device

#pragma region Selection

/* Find any queue family supporting the specific queue preferences.
*/
int anyQueueFamily(VkPhysicalDevice &device, VkQueueFlags* pref_queueFlag, int num_flag)
{
	std::vector<VkQueueFamilyProperties> queueFamilyProperties;		// Holds queue properties of corresponding physical device in physicalDevices
	ALLOC_QUERY(vkGetPhysicalDeviceQueueFamilyProperties, queueFamilyProperties, device);

	//Find queue matching the queue flags:
	for (int f = 0; f < num_flag; f++)
	{
		VkQueueFlags flag = pref_queueFlag[f];
		for (uint32_t i = 0; i < queueFamilyProperties.size(); ++i)
		{
			if (hasFlag(queueFamilyProperties[i].queueFlags, flag))
			{
				// Match with other properties ..?
				return i;
			}
		}
	}
	// No matching queue found
	return -1;
}
/* Find a queue family that exactly matches one of the preferences.
*/
int matchQueueFamily(VkPhysicalDevice &device, VkQueueFlags* pref_queueFlag, int num_flag)
{
	std::vector<VkQueueFamilyProperties> queueFamilyProperties;		// Holds queue properties of corresponding physical device in physicalDevices
	ALLOC_QUERY(vkGetPhysicalDeviceQueueFamilyProperties, queueFamilyProperties, device);

	//Find queue matching the queue flags:
	for (int f = 0; f < num_flag; f++)
	{
		VkQueueFlags flag = pref_queueFlag[f];
		for (uint32_t i = 0; i < queueFamilyProperties.size(); ++i)
		{
			if (matchFlag(queueFamilyProperties[i].queueFlags, flag))
			{
				// Match with other properties ..?
				return i;
			}
		}
	}
	// No matching queue found
	return -1;
}

/* Find a queue family that exactly matches one of the preferences, if nothing found the any family that supports the preference is selected.
device			<<	Physical device
pref_queueFlag	<<	List of queue flags ordered so that the first queue match to one bit flag is selected.
num_flag		<<	Number of flags in the ordered list.
*/
int pickQueueFamily(VkPhysicalDevice &device, VkQueueFlags* pref_queueFlag, int num_flag)
{
	int family = matchQueueFamily(device, pref_queueFlag, num_flag);
	if (family > -1)
		return family;
	return anyQueueFamily(device, pref_queueFlag, num_flag);
}

/* Check if there are a combination of queue families that supports all requested features. 
 * (does not guarantee support combinations). Returns 0 on success.
*/
int noQueueFamilySupport(VkQueueFlags feature, VkQueueFamilyProperties *family_list, size_t list_len)
{
	for (uint32_t i = 0; i < list_len; ++i)
		feature = rmvFlag(feature, family_list[i].queueFlags);
	return feature;
}

namespace vk
{
	/*	Determines if a physical device is suitable for the system. Return rank of the device, ranks greater then 0 will be available for selection. */
	typedef int(*isDeviceSuitable)(VkPhysicalDevice &device, VkPhysicalDeviceProperties &prop, VkPhysicalDeviceFeatures &feat, VkQueueFamilyProperties *queue_family_prop, size_t len_family_prop);

	/* Function selecting any dedicated device making a preference for discrete over integrated devices.
	*/
	int specifyAnyDedicatedDevice(VkPhysicalDevice &device, VkPhysicalDeviceProperties &prop, VkPhysicalDeviceFeatures &feat, VkQueueFamilyProperties *queue_family_prop, size_t len_family_prop)
	{
		if (prop.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			return 2;
		else if (prop.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
			return 1;
		return 0;
	}
}

/* Abstract device selection function using a ranking function to select device. 
instance		<<	Vulkan instance
deviceSpec		<<	Ranking function of available physical devices used to determine a suitable device.
queueSupportReq	<<	Flag verifying that (to be) ranked devices supports a set of queue families.  
result			>>	Selected device
return			>>	Positive: index of the related device, Negative: Error code
*/
int choosePhysicalDevice(VkInstance &instance, VkSurfaceKHR &surface, vk::isDeviceSuitable deviceSpec, VkQueueFlags queueSupportReq, VkPhysicalDevice &result)
{
	// Handles to the physical devices detected
	std::vector<VkPhysicalDevice> physicalDevices;
						
	// Query for suitable devices
	VkResult err;
	ALLOC_QUERY_ASSERT(err, vkEnumeratePhysicalDevices, physicalDevices, instance);
		if (physicalDevices.size() == 0)
			throw std::runtime_error("Failed to find GPUs with Vulkan support!");

	VkPhysicalDeviceProperties property;
	VkPhysicalDeviceFeatures feature;
	std::vector<VkQueueFamilyProperties> familyProperty;

	struct DevPair
	{
		int index, rank;
	};
	std::vector<DevPair> list;
	list.resize(physicalDevices.size());
	// Check for a discrete GPU
	for (uint32_t i = 0; i < physicalDevices.size(); ++i)
	{
		vkGetPhysicalDeviceProperties(physicalDevices[i], &property);									// Holds properties of corresponding physical device in physicalDevices
		vkGetPhysicalDeviceFeatures(physicalDevices[i], &feature);										// Holds features of corresponding physical device in physicalDevices

		ALLOC_QUERY(vkGetPhysicalDeviceQueueFamilyProperties, familyProperty, physicalDevices[i]);		// Holds queue properties of corresponding physical device in physicalDevices

		// Ensure the device can support specific queue families (and thus related operations)
		if (noQueueFamilySupport(queueSupportReq, familyProperty.data(), familyProperty.size()))
			continue;

		// Ensure the device can present on the specific surface (it is physically connected to the screen?).
		VkBool32 presentSupport = false;
		VkResult err = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevices[i], i, surface, &presentSupport);
		assert(err == VK_SUCCESS);
		if(!presentSupport)
			continue;

		// Find suitable device
		int rank = deviceSpec(physicalDevices[i], property, feature, familyProperty.data(), familyProperty.size());
		if (rank > 0)
			list.push_back({ (int)i, rank });
	}
	if (list.size() == 0)
	{
		//throw std::runtime_error("Failed to find physical device matching specification!");
		return -1;
	}
	//Select suitable device
	DevPair dev = list[0];
	for (size_t i = 1; i < list.size(); i++)
	{
		if (list[i].rank > dev.rank)
			dev = list[i];
	}
	//Return selected device
	result = physicalDevices[dev.index];
	return dev.index;
}


/* Find validation layers that are supported.
validationLayers	<<	Set of validation layers requested.
num_layer			<<	Number of layers in the set.
*/
std::vector<char*> checkValidationLayerSupport(char** validationLayers, size_t num_layer) {
	std::vector<VkLayerProperties> availableLayers;
	ALLOC_QUERY_NOPARAM(vkEnumerateInstanceLayerProperties, availableLayers);

	std::vector<char*> available;
	available.reserve(num_layer);
	for (size_t i = 0; i < num_layer; i++) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(validationLayers[i], layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (layerFound)
			available.push_back(validationLayers[i]);
	}

	return available;
}

#pragma endregion

/* Create a semaphore
*/
VkSemaphore createSemaphore(VkDevice device) {
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreInfo.flags = 0;
	semaphoreInfo.pNext = nullptr;
	VkSemaphore semaphore;
	if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore) != VK_SUCCESS)
		throw std::runtime_error("Failed to create semaphore!");
	return semaphore;
}
/* Create a fence
device		<<	...
signaled	<<	If the fence is initially set in a signaled state.
*/
VkFence createFence(VkDevice device, bool signaled)
{
	VkFenceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	createInfo.pNext = nullptr;
	// If flags contains VK_FENCE_CREATE_SIGNALED_BIT then the fence object is created in the signaled state. Otherwise it is created in the unsignaled state.
	createInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

	VkFence fence;
	if(vkCreateFence(device, &createInfo, nullptr, &fence) != VK_SUCCESS)
		throw std::runtime_error("Failed to create fence!");
	return fence;
}
/* Wait for a single fence until set (spin lock with timeout), the fence is then reset.
*/
void waitFence(VkDevice device, VkFence fence)
{
	while (vkWaitForFences(device, 1, &fence, VK_TRUE, 5) != VK_SUCCESS)
	{	}
	vkResetFences(device, 1, &fence);
}
/* Define creation of a single queue of the family type.
family	<<	Queue family index
prio	<<	Priority of commands submitted in the queue
return		>>	A defined VkDeviceQueueCreateInfo struct
*/
VkDeviceQueueCreateInfo defineQueue(int family, float prio)
{
	VkDeviceQueueCreateInfo queueInfo;
	queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueInfo.pNext = nullptr;
	queueInfo.flags = 0;
	queueInfo.queueFamilyIndex = family;
	queueInfo.queueCount = 1;
	queueInfo.pQueuePriorities = &prio;	// Defines the prio for each created queue
	return queueInfo;
}
/* Define creation of a set of queues of the family type.
family	<<	Queue family index
prio	<<	List of priority values of each queue.
num_queues	<<	Number of queues of the family to create (note the priority list must be of same size).
return		>>	A defined VkDeviceQueueCreateInfo struct 
*/
VkDeviceQueueCreateInfo defineQueues(int family, float *prio, uint32_t num_queues)
{
	VkDeviceQueueCreateInfo queueInfo;
	queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueInfo.pNext = nullptr;
	queueInfo.flags = 0;
	queueInfo.queueFamilyIndex = family;
	queueInfo.queueCount = num_queues;
	queueInfo.pQueuePriorities = prio;	// Defines the prio for each created queue
	return queueInfo;
}

#pragma endregion

#pragma region Swapchain

VkPresentModeKHR chooseSwapPresentMode(VkPhysicalDevice &device, VkSurfaceKHR &surface, VkPresentModeKHR *prefered_modes, size_t num_prefered) {

	// Find available present modes of the device
	std::vector<VkPresentModeKHR> presentModes;
	VkResult err;
	ALLOC_QUERY_ASSERT(err, vkGetPhysicalDeviceSurfacePresentModesKHR, presentModes, device, surface);
#ifdef _DEBUG
	// Output present modes:
	std::cout << presentModes.size() << " present mode(s)\n";
	for (size_t i = 0; i < presentModes.size() - 1; i++)
		std::cout << presentModes[i] << ", ";
	std::cout << presentModes[presentModes.size() - 1] << "\n";
#endif

	// Find an acceptable present mode
	for (size_t i = 0; i < num_prefered; i++)
	{
		if (hasMode(prefered_modes[i], presentModes.data(), presentModes.size()))
			return prefered_modes[i];
	}
	// 'Guaranteed' to exist
	return VK_PRESENT_MODE_FIFO_KHR;
}


/* Create a frame buffer. A collection of swapchain image views that define the framebuffer targets.
pass			<<
frameDim		<<	Dimension of the frame buffer
attachments		<<	Attached frame buffer image targets.
num_attachment	<<	Number of attached images.
*/
VkFramebuffer createFramebuffer(VkDevice device, VkRenderPass pass, VkExtent2D frameDim, VkImageView *attachments, uint32_t num_attachment) 
{

	VkFramebufferCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	info.renderPass = pass;
	info.attachmentCount = num_attachment;
	info.pAttachments = attachments;
	info.width = frameDim.width;
	info.height = frameDim.height;
	info.layers = 1;

	VkFramebuffer frameBuffer;
	if (vkCreateFramebuffer(device, &info, nullptr, &frameBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create framebuffer!");
	}
	return frameBuffer;
}

VkAttachmentDescription defineFramebufColor(VkFormat swapChainImgFormat)
{
	VkAttachmentDescription colAttach = {};
	colAttach.format = swapChainImgFormat;
	colAttach.samples = VK_SAMPLE_COUNT_1_BIT;
	colAttach.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;					// Clear col/depth buff before rendering.
	colAttach.storeOp = VK_ATTACHMENT_STORE_OP_STORE;				// Store col/depth buff data for access/present.
	colAttach.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colAttach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colAttach.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colAttach.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	return colAttach;
}
VkAttachmentDescription defineFramebufDepth(VkFormat depthImgFormat)
{
	VkAttachmentDescription depthAttach = {};
	depthAttach.format = depthImgFormat;
	depthAttach.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttach.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;					// Clear col/depth buff before rendering.
	depthAttach.storeOp = VK_ATTACHMENT_STORE_OP_STORE;				// Store col/depth buff data for access/present.
	depthAttach.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttach.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttach.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	return depthAttach;
}

/* Create a simple single render pass with attached color buffer.
*/
VkRenderPass createRenderPass_SingleColor(VkDevice device, VkFormat swapChainImgFormat) {

	VkAttachmentDescription colAttach = defineFramebufColor(swapChainImgFormat);
	// Referenced renderbuffer target
	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = NULL;

	// Specify dependency for transitioning the image layout for rendering. 
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colAttach;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	VkRenderPass pass;
	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &pass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
	return pass;
}

/* Create a simple single render pass with attached color buffer and depth buffer.
*/
VkRenderPass createRenderPass_SingleColorDepth(VkDevice device, VkFormat swapChainImgFormat, VkFormat depthFormat) {
	const int num_attach = 2;
	VkAttachmentDescription attach[num_attach] = {
		defineFramebufColor(swapChainImgFormat),
		defineFramebufDepth(depthFormat)
	};

	// Referenced renderbuffer target
	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	// Specify dependency for transitioning the image layout for rendering. 
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = num_attach;
	renderPassInfo.pAttachments = attach;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	VkRenderPass pass;
	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &pass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
	return pass;
}

/* Find a supported image format.
physDevice	<<	Physical device
candidates	<<	List of VkFormats checked for support prioritized by order.
num_cand	<<	Number of candidates in the list.
tiling		<<	Type of image tiling specified.
features	<<	Tiling feature flags queried for.
return		>>	First format in the candidate list that fits the specified params.
*/
VkFormat findSupportedFormat(VkPhysicalDevice physDevice, const VkFormat* candidates, size_t num_cand, VkImageTiling tiling, VkFormatFeatureFlags features) {
	for (size_t i = 0; i < num_cand; i++) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physDevice, candidates[i], &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return candidates[i];
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return candidates[i];
		}
	}
	throw std::runtime_error("Failed to find supported format!");
}

/* Find a suitable depth format related to the physical device
*/
VkFormat findDepthFormat(VkPhysicalDevice physDevice) {
	const size_t num_cand = 3;
	VkFormat list[num_cand] = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
	return findSupportedFormat(
		physDevice, 
		list, num_cand,
		VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);

}

/* Check if a specific image format has a stencil component.
*/
bool hasStencilComponent(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}
#pragma endregion

#pragma region Memory

#pragma region Buffer

/* Create a vulkan buffer of specific byte size and type.
byte_size		<<	Byte size of the buffer.
return			>>	The vertex buffer handle.
*/
VkBuffer createBuffer(VkDevice device, size_t byte_size, VkBufferUsageFlags usage, uint32_t queueCount, uint32_t *queueFamilyIndices)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = byte_size;
	bufferInfo.flags = 0;
	bufferInfo.usage = usage;
	if (queueCount <= 1)
		// Indicates it'll only be accessed by a single queue at a time (presumably only use this option if it will only be used by one queue)
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	else
		// Indicate multiple queues will concurrently use the buffer.
		bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
	// Since exclusive the queue params are redundant:
	bufferInfo.queueFamilyIndexCount = queueCount;
	bufferInfo.pQueueFamilyIndices = queueFamilyIndices;

	VkBuffer buffer;
	if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create buffer!");
	}
	return buffer;
}

/* Defines a VkVertexInputBindingDescription from the params. Used to bind (associate) a vertex buffer layout associated with the pipeline.
bind_index		<<	Binding index for the description and(/or?) buffer.
vertex_bytes	<<	Number of bytes per vertex in the buffer.
inputRate		<<	Defines how vertices are read and if instancing should be used. VK_VERTEX_INPUT_RATE_VERTEX, VK_VERTEX_INPUT_RATE_INSTANCE. 
*/
VkVertexInputBindingDescription defineVertexBinding(uint32_t bind_index, uint32_t vertex_bytes, VkVertexInputRate inputRate)
{
	VkVertexInputBindingDescription desc = {};
	desc.binding = bind_index;
	desc.stride = vertex_bytes;
	desc.inputRate = inputRate;
	return desc;
}

/* Define a vertex attribute from the params.
bind_index	<<	The binding index of the related VkVertexInputBindingDescription (and hence vertex buffer) the attribute is associated with.
loc_index	<<	The shader's input location for the attribute.
format		<<	VkFormat specifying the data type of the attribute (specified as color channels etc..).
offset		<<	Specifies the byte offset in the vertex in SoA format.
*/
VkVertexInputAttributeDescription defineVertexAttribute(uint32_t bind_index, uint32_t loc_index, VkFormat format, uint32_t attri_offset)
{
	VkVertexInputAttributeDescription attri = {};
	attri.binding = bind_index;
	attri.location = loc_index;
	attri.format = format;
	attri.offset = attri_offset;
	return attri;
}

#pragma endregion

#pragma region Image/Texture

/* Create a 2D texture image of specific size and format.
*/
VkImage createTexture2D(VkDevice device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling)
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.format = format;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;

	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	// This image is used for sampling!...
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0; // Optional
	imageInfo.queueFamilyIndexCount = 0;
	imageInfo.pQueueFamilyIndices = nullptr;

	VkImage texture;
	VkResult result = vkCreateImage(device, &imageInfo, nullptr, &texture);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}
	return texture;
}
VkImage createDepthBuffer(VkDevice device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling)
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.format = format;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;

	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	// This image is used for sampling!...
	imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0; // Optional
	imageInfo.queueFamilyIndexCount = 0;
	imageInfo.pQueueFamilyIndices = nullptr;

	VkImage texture;
	VkResult result = vkCreateImage(device, &imageInfo, nullptr, &texture);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}
	return texture;
}

VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageViewType viewType) {
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = viewType;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;
	viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	VkImageView imageView;
	VkResult result = vkCreateImageView(device, &viewInfo, nullptr, &imageView);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}

	return imageView;
}

/* Create a simple sampler with the base parameters set
*/
VkSampler createSampler(VkDevice device, VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode wrap_s, VkSamplerAddressMode wrap_t)
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.pNext = nullptr;
	samplerInfo.flags = 0;
	// Filters
	samplerInfo.magFilter = magFilter;
	samplerInfo.minFilter = minFilter;
	// Wrap mode
	samplerInfo.addressModeU = wrap_s;
	samplerInfo.addressModeV = wrap_t;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	// Anisotropy
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 1.0;
	//Mipmapping
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;
	// Misc
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	VkSampler sampler;
	VkResult err = vkCreateSampler(device, &samplerInfo, nullptr, &sampler);
	if (err != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
	}
	return sampler;
}
/* Check for a list of image formats supported.
*/
void checkValidImageFormats(VkPhysicalDevice device)
{
	const int NUM_FORMAT = 2;
	VkFormat FORMATS[NUM_FORMAT] =
	{
		VK_FORMAT_R8G8B8_UNORM,
		VK_FORMAT_R8G8B8_UINT
	};
	char* NAMES[NUM_FORMAT] =
	{
		"VK_FORMAT_R8G8B8_UNORM",
		"VK_FORMAT_R8G8B8_UINT"
	};
	VkFormatProperties prop;
	for (int i = 0; i < NUM_FORMAT; i++)
	{
		vkGetPhysicalDeviceFormatProperties(device, FORMATS[i], &prop);
		if (prop.optimalTilingFeatures == 0)
			std::cout << NAMES[i] << " not supported\n";
		else
			std::cout << NAMES[i] << " supported\n";
	}
}

#pragma endregion

/* Find a memory type on the device mathcing the specification
*/
uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {

	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	// Iterate the different types matching the filter mask and find one that matches the properties:
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && matchFlag(memProperties.memoryTypes[i].propertyFlags, properties)) {
			return i;
		}
	}
	throw std::runtime_error("failed to find suitable memory type!");
}
/* Allocate physical memory on the device of specific parameters.
device			<< Handle to the device.
physicalDevice	<< Handle to the physical device.
buffer			<< Related buffer.
requirements	<< The memory requirements.
properties		<< The properties the memory should have (dependent on buffer and how it's used).
*/
VkDeviceMemory allocPhysicalMemory(VkDevice device, VkPhysicalDevice physicalDevice, VkMemoryRequirements requirements, VkMemoryPropertyFlags properties)
{
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = requirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, requirements.memoryTypeBits, properties);

	//Allocate:
	// *Note that number allocations is limited, hence a custom allocator is required to allocate chunks for multiple buffers. 
	// *One possible solution is to use VulkanMemoryAllocator library.
	VkDeviceMemory mem;
	if (vkAllocateMemory(device, &allocInfo, nullptr, &mem) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate vertex buffer memory!");
	}
	return mem;
}
/* Allocate physical memory on the device for a buffer object.
device			<< Handle to the device.
physicalDevice	<< Handle to the physical device.
buffer			<< Buffer specifying the memory req.
properties		<< The properties the memory should have (dependent on buffer and how it's used).
bindToBuffer	<< If the memory should be bound to the buffer
*/
VkDeviceMemory allocPhysicalMemory(VkDevice device, VkPhysicalDevice physicalDevice, VkBuffer buffer, VkMemoryPropertyFlags properties, bool bindToBuffer)
{
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	//Allocate:
	// *Note that the number of allocations are limited!
	VkDeviceMemory mem = allocPhysicalMemory(device, physicalDevice, memRequirements, properties);
	// Bind to buffer.
	if(bindToBuffer)
		vkBindBufferMemory(device, buffer, mem, 0);
	return mem;
}
/* Allocate physical memory on the device for a buffer object.
device			<< Handle to the device.
physicalDevice	<< Handle to the physical device.
image			<< Image specifying the memory req.
properties		<< The properties the memory should have (dependent on buffer and how it's used).
bindToImage		<< If the memory should be bound to the image parameter.
*/
VkDeviceMemory allocPhysicalMemory(VkDevice device, VkPhysicalDevice physicalDevice, VkImage image, VkMemoryPropertyFlags properties, bool bindToImage)
{
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);

	//Allocate:
	// *Note that the number of allocations are limited!
	VkDeviceMemory mem = allocPhysicalMemory(device, physicalDevice, memRequirements, properties);
	// Bind to buffer.
	if (bindToImage)
		vkBindImageMemory(device, image, mem, 0);
	return mem;
}


#pragma endregion



#pragma region Command
/* Create a command buffer for re-use.
device		<<	The device
commandPool <<	Pool to allocate command buffer from.
bufferSize	<<	Max number of commands possible in the buffer.
return		>>	The created command buffer.
*/
VkCommandBuffer allocateCmdBuf(VkDevice device, VkCommandPool commandPool)
{
	// Create command buffer
	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.pNext = nullptr;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandPool = commandPool;
	commandBufferAllocateInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	VkResult result = vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create command buffer for staging.");
	return commandBuffer;
}
/* Call vkBeginCommandBuffer with single submit usage.
*/
void beginCmdBuf(VkCommandBuffer cmdBuf, VkFlags flag)
{
	// Begin recording into command buffer
	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pNext = nullptr;
	commandBufferBeginInfo.flags = flag; //Recording will be submitted once.
	commandBufferBeginInfo.pInheritanceInfo = nullptr;

	VkResult result = vkBeginCommandBuffer(cmdBuf, &commandBufferBeginInfo);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to begin command buffer.");
}
/* Create a command buffer for single time use.
device		<<	The device
commandPool <<	Pool to allocate command buffer from.
return		>>	The created command buffer.
*/
VkCommandBuffer beginSingleCommand(VkDevice device, VkCommandPool commandPool)
{
	VkCommandBuffer cmdBuf = allocateCmdBuf(device, commandPool);
	beginCmdBuf(cmdBuf);
	return cmdBuf;
}

/* Submit, wait for finish and clean-up a single time use command buffer.
device		<<	The device
queue		<<	The queue to submit the buffer.
commandPool	<<	Command pool to free the command buffer from.
commandBuf	<<	The command buffer to submit.
submitCount	<<	Number of commands to submit.
*/
void endSingleCommand_Wait(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkCommandBuffer commandBuf)
{
	// End command recording
	vkEndCommandBuffer(commandBuf);

	// Submit command buffer to queue
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuf;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = nullptr;

	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);		// Wait until the copy is complete
	vkFreeCommandBuffers(device, commandPool, 1, &commandBuf);
}
/* Submit a single time use command buffer.
device		<<	The device
queue		<<	The queue to submit the buffer.
commandBuf	<<	The command buffer to submit.
submitCount	<<	Number of commands to submit.
*/
void endSingleCommand(VkDevice device, VkQueue queue, VkCommandBuffer commandBuf, VkFence fence)
{
	// End command recording
	vkEndCommandBuffer(commandBuf);

	// Submit command buffer to queue
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuf;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = nullptr;

	vkQueueSubmit(queue, 1, &submitInfo, fence);
}

/* Wait for queue to idle then release command buffer.
*/
void releaseCommandBuffer(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkCommandBuffer commandBuf)
{
	vkQueueWaitIdle(queue);		// Wait until the copy is complete
	vkFreeCommandBuffers(device, commandPool, 1, &commandBuf);
}


#pragma endregion

#pragma region Descriptors

/* Fill a VkWriteDescriptorSet with VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER write info.
writeInfo		<<	Struct filled with the params.
dstSet			<<	Destination descriptor set (set updated)
dstBinding		<<	Descriptor binding within the set that is updated.
dstArrayElement	<<	Destination element within the array.
descriptorCount	<<	Number of descriptors updated (number of elements in the imageInfo array)
imageInfo		<<	Array of ImageInfo updated within the descriptor set.
*/
void writeDescriptorStruct_IMG_COMBINED(VkWriteDescriptorSet &writeInfo, VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t dstArrayElem, uint32_t descriptorCount, VkDescriptorType type, VkDescriptorImageInfo *imageInfo)
{
	writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeInfo.dstSet = dstSet;
	writeInfo.dstBinding = dstBinding;
	writeInfo.dstArrayElement = dstArrayElem;
	writeInfo.descriptorCount = descriptorCount;
	writeInfo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeInfo.pImageInfo = imageInfo;
	writeInfo.pBufferInfo = nullptr;
	writeInfo.pTexelBufferView = nullptr;
}

/* Fill a VkWriteDescriptorSet with VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER write info.
writeInfo		<<	Struct filled with the params.
dstSet			<<	Destination descriptor set (set updated)
dstBinding		<<	Descriptor binding within the set that is updated.
dstArrayElement	<<	Destination element within the array.
descriptorCount	<<	Number of descriptors updated (number of elements in the imageInfo array)
imageInfo		<<	Array of VkDescriptorImageInfo updated within the descriptor set.
*/
void writeDescriptorStruct_IMG_COMBINED(VkWriteDescriptorSet &writeInfo, VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t dstArrayElem, uint32_t descriptorCount, VkDescriptorImageInfo *imageInfo)
{
	writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeInfo.dstSet = dstSet;
	writeInfo.dstBinding = dstBinding;
	writeInfo.dstArrayElement = dstArrayElem;
	writeInfo.descriptorCount = descriptorCount;
	writeInfo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeInfo.pImageInfo = imageInfo;
	writeInfo.pBufferInfo = nullptr;
	writeInfo.pTexelBufferView = nullptr;
}
/* Fill a VkWriteDescriptorSet with VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER write info.
writeInfo		<<	Struct filled with the params.
dstSet			<<	Destination descriptor set (set updated)
dstBinding		<<	Descriptor binding within the set that is updated.
dstArrayElement	<<	Destination element within the array.
descriptorCount	<<	Number of descriptors updated (number of elements in the bufferInfo array)
bufferInfo		<<	Array of VkDescriptorBufferInfo updated within the descriptor set.
*/
void writeDescriptorStruct_UNI_BUFFER(VkWriteDescriptorSet &writeInfo, VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t dstArrayElem, uint32_t descriptorCount, VkDescriptorBufferInfo* bufferInfo)
{
	writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeInfo.dstSet = dstSet;
	writeInfo.dstBinding = dstBinding;
	writeInfo.dstArrayElement = dstArrayElem;
	writeInfo.descriptorCount = descriptorCount;
	writeInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeInfo.pImageInfo = nullptr;
	writeInfo.pBufferInfo = bufferInfo;
	writeInfo.pTexelBufferView = nullptr;
}
/* Write layout binding
*/
void writeLayoutBinding(VkDescriptorSetLayoutBinding &layoutBinding, uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage)
{
	layoutBinding.binding = binding;
	layoutBinding.descriptorCount = 1;
	layoutBinding.descriptorType = type;
	layoutBinding.pImmutableSamplers = nullptr;
	layoutBinding.stageFlags = stage;
}

/* Create a VkDescriptorSetLayout from the bindings.
*/
VkDescriptorSetLayout createDescriptorLayout(VkDevice device, VkDescriptorSetLayoutBinding *bindings, size_t num_binding)
{
	VkDescriptorSetLayoutCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.bindingCount = (uint32_t)num_binding;
	createInfo.pBindings = bindings;

	VkDescriptorSetLayout layout;
	VkResult result = vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &layout);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor set layout.");
	return layout;
}

/* Create a descriptor pool from the params.
device	<< 
sizeTypes	<<	Array of descriptors defining the descriptors size that is allocated from the pool.
num_types	<<	Length of the 'sizeTypes' array
poolSize	<<	Max number of descriptors sets allocated from the pool.
*/
VkDescriptorPool createDescriptorPool(VkDevice device, VkDescriptorPoolSize *sizeTypes, uint32_t num_types, uint32_t poolSize)
{

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.pNext = nullptr;
	descriptorPoolCreateInfo.flags = 0;
	descriptorPoolCreateInfo.maxSets = poolSize;
	descriptorPoolCreateInfo.poolSizeCount = num_types;
	descriptorPoolCreateInfo.pPoolSizes = sizeTypes;

	VkDescriptorPool pool;
	VkResult result = vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &pool);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor pool.");
	return pool;
}

/* Create a descriptor pool for descriptor sets of single items.
*/
VkDescriptorPool createDescriptorPoolSingle(VkDevice device, VkDescriptorType type, uint32_t poolSize)
{
	// Describes how many of every descriptor type can be created in the pool
	VkDescriptorPoolSize descriptorSizes;
	descriptorSizes.type = type;
	// Number of descriptors of this type allocated inside the pool (for our pool of single types this equals the number of descriptor sets allocated).
	descriptorSizes.descriptorCount = poolSize;
	// Create pool
	return createDescriptorPool(device, &descriptorSizes, 1, poolSize);
}

VkDescriptorSet createDescriptorSet(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout *layouts)
{
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.pNext = nullptr;
	descriptorSetAllocateInfo.descriptorPool = pool;
	descriptorSetAllocateInfo.descriptorSetCount = 1;
	descriptorSetAllocateInfo.pSetLayouts = layouts;

	// Create a descriptor set for descriptorSet
	VkDescriptorSet set;
	VkResult err = vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &set);
#ifdef _DEBUG
	if (err != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor set.");
#endif
	return set;
}
#pragma endregion


#pragma region Pipeline


/* Define a viewport of specific size.
*/
VkViewport defineViewport(float width, float height)
{
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = width;
	viewport.height = height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	return viewport;
}
/* Define a viewport from it's parameters.
*/
VkViewport defineViewport(float x, float y, float width, float height, float minDepth, float maxDepth)
{
	VkViewport viewport = {};
	viewport.x = x;
	viewport.y = y;
	viewport.width = width;
	viewport.height = height;
	viewport.minDepth = minDepth;
	viewport.maxDepth = maxDepth;
	return viewport;
}

/* Define a scissor rectangle from bounds.
*/
VkRect2D defineScissorRect(int32_t x, int32_t y, uint32_t width, uint32_t height)
{
	VkRect2D scissor = {};
	scissor.offset = { x, y };
	scissor.extent = { width, height };
	return scissor;
}
/* Define a scissor rectangle to fit the viewport.
*/
VkRect2D defineScissorRect(VkViewport &viewport)
{
	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = { (uint32_t)viewport.width, (uint32_t)viewport.height };
	return scissor;
}

/* Define the VkPipelineViewportStateCreateInfo from the params.
*/
VkPipelineViewportStateCreateInfo defineViewportState(VkViewport *viewport, VkRect2D *scissor)
{
	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.pNext = nullptr;
	viewportState.flags = 0;
	viewportState.viewportCount = 1;
	viewportState.pViewports = viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = scissor;
	return viewportState;
}

/* Define how vertex data is read from buffers.
primitiveType	<<	Define the type of primitives used (TRIANGLE_LIST, TRIANGLE_STRIP, LINE_LIST...)
indexLoopEnable	<<	Define if primitive strips should be looped at a certain index (must be associated with STRIP type and use index buffer).
*/
VkPipelineInputAssemblyStateCreateInfo defineInputAssembly(VkPrimitiveTopology primitiveType, VkBool32 indexLoopEnable)
{
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.pNext = nullptr;
	inputAssembly.flags = 0;
	inputAssembly.topology = primitiveType;
	inputAssembly.primitiveRestartEnable = indexLoopEnable;
	return inputAssembly;
}

/* Specify multisampling to be off (use only a single sample)
*/
VkPipelineMultisampleStateCreateInfo defineMultiSampling_OFF()
{
	VkPipelineMultisampleStateCreateInfo multisampleState = {};
	multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleState.pNext = nullptr;
	multisampleState.flags = 0;
	multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleState.sampleShadingEnable = VK_FALSE;
	multisampleState.minSampleShading = 0.0f;
	multisampleState.pSampleMask = nullptr;
	multisampleState.alphaToCoverageEnable = VK_FALSE;
	multisampleState.alphaToOneEnable = VK_FALSE;
	return multisampleState;
}

/* Define a blend state from the params.
*/
VkPipelineColorBlendStateCreateInfo defineBlendState(VkPipelineColorBlendAttachmentState *blendStateAttachments, uint32_t num_attachments, glm::vec4 blendConstants)
{
	VkPipelineColorBlendStateCreateInfo blendState = {};
	blendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blendState.pNext = nullptr;
	blendState.flags = 0;
	blendState.logicOpEnable = VK_FALSE;
	blendState.logicOp = VK_LOGIC_OP_COPY;
	blendState.attachmentCount = num_attachments;
	blendState.pAttachments = blendStateAttachments;
	blendState.blendConstants[0] = blendConstants.r;
	blendState.blendConstants[1] = blendConstants.g;
	blendState.blendConstants[2] = blendConstants.b;
	blendState.blendConstants[3] = blendConstants.a;
	return blendState;
}
/* Define a blend state with logic operation when updating the framebuffer (note that only framebuffer format must be of integer type and not in float or sRGB format).
*/
VkPipelineColorBlendStateCreateInfo defineBlendState_LogicOp(VkPipelineColorBlendAttachmentState *blendStateAttachments, uint32_t num_attachments, VkLogicOp logic_op, glm::vec4 blendConstants)
{
	VkPipelineColorBlendStateCreateInfo blendState = {};
	blendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blendState.pNext = nullptr;
	blendState.flags = 0;
	blendState.logicOpEnable = VK_TRUE;
	blendState.logicOp = logic_op;
	blendState.attachmentCount = num_attachments;
	blendState.pAttachments = blendStateAttachments;
	blendState.blendConstants[0] = blendConstants.r;
	blendState.blendConstants[1] = blendConstants.g;
	blendState.blendConstants[2] = blendConstants.b;
	blendState.blendConstants[3] = blendConstants.a;
	return blendState;
}


/* Define a simple uniform layout using uniform buffers (no push constants).
*/
VkPipelineLayout createPipelineLayout(VkDevice device, VkDescriptorSetLayout *descriptorSet, uint32_t num_descriptors)
{
	VkPipelineLayoutCreateInfo layout = {};
	layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layout.pNext = nullptr;
	layout.flags = 0;
	layout.setLayoutCount = num_descriptors;
	layout.pSetLayouts = descriptorSet;
	layout.pushConstantRangeCount = 0;
	layout.pPushConstantRanges = nullptr;

	VkPipelineLayout pipelineLayout;
	VkResult result = vkCreatePipelineLayout(device, &layout, nullptr, &pipelineLayout);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create pipeline layout.");
	return pipelineLayout;
}

/* Define a shader used for a shader stage.
stage		<<	Shader stage
shader		<<	Shader bound to the stage.
entryFunc	<<	Name of the entry point function in the shader.
*/
VkPipelineShaderStageCreateInfo defineShaderStage(VkShaderStageFlagBits stage, VkShaderModule shader, const char* entryFunc)
{
	VkPipelineShaderStageCreateInfo shaderStage = {};
	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.pNext = NULL;
	shaderStage.flags = 0;
	shaderStage.stage = stage;
	shaderStage.module = shader;
	shaderStage.pName = entryFunc;
	shaderStage.pSpecializationInfo = NULL;
	return shaderStage;
}

/* Fill a VkPipelineVertexInputStateCreateInfo from the params.
bindings	<<	Vertex buffer bindings used in the pipeline.
num_buffers	<<	Number of buffer bindings.
attributes	<<	Vertex attributes associated with the buffers (and used in pipeline).
num_attri	<<	Number of attributes.
*/
VkPipelineVertexInputStateCreateInfo defineVertexBufferBindings(VkVertexInputBindingDescription *bindings, uint32_t num_buffers, VkVertexInputAttributeDescription *attributes, uint32_t num_attri)
{
	VkPipelineVertexInputStateCreateInfo bufferBindings = {};
	bufferBindings.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	bufferBindings.pNext = nullptr;
	bufferBindings.flags = 0;
	bufferBindings.vertexBindingDescriptionCount = num_buffers;
	bufferBindings.pVertexBindingDescriptions = bindings;
	bufferBindings.vertexAttributeDescriptionCount = num_attri;
	bufferBindings.pVertexAttributeDescriptions = attributes;
	return bufferBindings;
}
/* Define a simple rasterization state from params.
*/
VkPipelineRasterizationStateCreateInfo defineRasterizationState(uint32_t rasterFlags, VkCullModeFlags cullMode, float lineWidth)
{
	VkPipelineRasterizationStateCreateInfo rasterizationState = {};
	rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationState.pNext = nullptr;
	rasterizationState.flags = 0;
	rasterizationState.depthClampEnable = (VkBool32)hasFlag(rasterFlags, DEPTH_CLAMP_BIT);
	rasterizationState.rasterizerDiscardEnable = (VkBool32)hasFlag(rasterFlags, NO_RASTERIZATION_BIT);
	rasterizationState.polygonMode = hasFlag(rasterFlags, WIREFRAME_BIT) ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
	rasterizationState.cullMode = cullMode;
	rasterizationState.frontFace = hasFlag(rasterFlags, CLOCKWISE_FACE_BIT) ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationState.lineWidth = lineWidth; // For line rendering
											  
	// Depth bias
	rasterizationState.depthBiasEnable = VK_FALSE;
	rasterizationState.depthBiasConstantFactor = 0.0f;
	rasterizationState.depthBiasClamp = 0.0f;
	rasterizationState.depthBiasSlopeFactor = 1.0f;
	return rasterizationState;
}

/* Define a basic depth stencil create info.
*/
VkPipelineDepthStencilStateCreateInfo defineDepthState()
{
	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {}; // Optional
	depthStencil.back = {}; // Optional
	return depthStencil;
}

#pragma endregion

#endif
