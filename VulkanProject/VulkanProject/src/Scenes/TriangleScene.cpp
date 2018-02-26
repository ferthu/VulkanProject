#include "Scenes/TriangleScene.h"
#include "VulkanRenderer.h"
#include "Stuff/RandomGenerator.h"

TriangleScene::TriangleScene()
{
}


TriangleScene::~TriangleScene()
{
	delete techniqueA;
	delete triShader;
	delete triBuffer;
}



void TriangleScene::initialize(VulkanRenderer *handle)
{
	Scene::initialize(handle);
	triShader = new ShaderVulkan("testShaders", _renderHandle);
	triShader->setShader("resource/trishader/VertexShader.glsl", ShaderVulkan::ShaderType::VS);
	triShader->setShader("resource/trishader/FragmentShader.glsl", ShaderVulkan::ShaderType::PS);
	std::string err;
	triShader->compileMaterial(err);


	// Create testing vertex buffer
	const uint32_t NUM_TRIS = 10;
	mf::RandomGenerator rnd;
	rnd.seedGenerator();
	glm::vec4 testTriangles[NUM_TRIS * 3];
	mf::distributeTriangles(rnd, 1.f, NUM_TRIS, glm::vec2(0.1f, 0.5f), testTriangles, nullptr);

	triBuffer = new VertexBufferVulkan(_renderHandle, sizeof(glm::vec4) * NUM_TRIS * 3, VertexBufferVulkan::DATA_USAGE::STATIC);
	triVertexBinding = VertexBufferVulkan::Binding(triBuffer, sizeof(glm::vec4), NUM_TRIS * 3, 0);
	triBuffer->setData(testTriangles, triVertexBinding.byteSize(), 0);

	makeTechniqueA();
}

void TriangleScene::makeTechniqueA()
{

	const uint32_t NUM_BUFFER = 1;
	const uint32_t NUM_ATTRI = 1;
	VkVertexInputBindingDescription vertexBufferBindings[NUM_BUFFER] =
	{
		defineVertexBinding(0, 16),
		/*defineVertexBinding(NORMAL, 16),
		defineVertexBinding(TEXTCOORD, 8)*/
	};
	VkVertexInputAttributeDescription vertexAttributes[NUM_ATTRI] =
	{
		defineVertexAttribute(0, 0, VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT, 0),
		/*defineVertexAttribute(NORMAL, NORMAL, VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT, 0),
		defineVertexAttribute(TEXTCOORD, TEXTCOORD, VkFormat::VK_FORMAT_R32G32_SFLOAT, 0)*/
	};
	VkPipelineVertexInputStateCreateInfo vertexBindings =
		defineVertexBufferBindings(vertexBufferBindings, NUM_BUFFER, vertexAttributes, NUM_ATTRI);
	techniqueA = new TechniqueVulkan(triShader, _renderHandle, _renderHandle->getFramePass(), vertexBindings);
}

void TriangleScene::frame(VkCommandBuffer cmdBuf)
{

	vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, techniqueA->pipeline);

	VkDeviceSize offsets = 0;
	triVertexBinding.bind(0);
	vkCmdDraw(cmdBuf, (uint32_t)triVertexBinding.numElements, 1, 0, 0);

}


void TriangleScene::defineDescriptorLayout(VkDevice device, std::vector<VkDescriptorSetLayout> &layout)
{
	layout.resize(1);
	VkDescriptorSetLayoutBinding binding;
	writeLayoutBinding(binding, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	layout[0] = createDescriptorLayout(device, &binding, 1);
}