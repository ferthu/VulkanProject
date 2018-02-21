#pragma once
#include "../Material.h"
#include <GL/glew.h>
#include <vector>
#include "ConstantBufferVulkan.h"
#include "VulkanRenderer.h"

class VulkanRenderer;

#define DBOUTW( s )\
{\
std::wostringstream os_;\
os_ << s;\
OutputDebugStringW( os_.str().c_str() );\
}

#define DBOUT( s )\
{\
std::ostringstream os_;\
os_ << s;\
OutputDebugString( os_.str().c_str() );\
}

// use X = {Program or Shader}
#define INFO_OUT(S,X) { \
char buff[1024];\
memset(buff, 0, 1024);\
glGet##X##InfoLog(S, 1024, nullptr, buff);\
DBOUTW(buff);\
}

// use X = {Program or Shader}
#define COMPILE_LOG(S,X,OUT) { \
char buff[1024];\
memset(buff, 0, 1024);\
glGet##X##InfoLog(S, 1024, nullptr, buff);\
OUT=std::string(buff);\
}

const uint32_t MAX_DESCRIPTOR_TYPES = 2; //2 for this solution...
const uint32_t MAX_MATERIAL_DESCRIPTORS = 8;

class MaterialVulkan :
	public Material
{
	friend VulkanRenderer;

public:
	MaterialVulkan(const std::string& name, VulkanRenderer *renderHandle);
	~MaterialVulkan();
	void setShader(const std::string& shaderFileName, ShaderType type);
	void removeShader(ShaderType type);

	void setDiffuse(Color c);

	int compileMaterial(std::string& errString);
	void addConstantBuffer(std::string name, unsigned int location);

	void updateConstantBuffer(const void* data, size_t size, unsigned int location);

	int enable();

	void disable();

	// Returns true if the defines for shaderType includes searchString
	bool hasDefine(Material::ShaderType shaderType, std::string searchString);
	
	VkShaderModule vertexShader;
	VkShaderModule fragmentShader;
	
private:
	std::string name;
	VulkanRenderer* _renderHandle;	// Pointer to the renderer that created this material
	std::map<unsigned int, ConstantBuffer*> constantBuffers;
	bool spawned;
	
		
	int createShaders();
	void destroyShaderObjects();
	std::string assembleShader(Material::ShaderType type);
	std::string assembleDefines(Material::ShaderType type);
	std::string runCompiler(Material::ShaderType type, std::string inputFileName);
	std::vector<char> loadSPIR_V(std::string fileName);

};

#pragma once
