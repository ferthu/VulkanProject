#pragma once
#include <vector>
#include <map>
#include <set>
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

class ShaderVulkan
{
public:
	enum class ShaderType { VS = 0, PS = 1, GS = 2, CS = 3 };
	ShaderVulkan(const std::string& name, VulkanRenderer *renderHandle);
	~ShaderVulkan();
	void setShader(const std::string& shaderFileName, ShaderType type);


	int compileMaterial(std::string& errString);

	VkShaderModule vertexShader, fragmentShader, compShader;
	
private:
	std::string name;
	VulkanRenderer* _renderHandle;	// Pointer to the renderer that created this material

	std::map<ShaderType, std::string> shaderFileNames;
	std::map<ShaderType, std::set<std::string>> shaderDefines;

	int createShaders();
	int createPipeShader();
	int createComputeShader();

	void destroyShaderObjects();
	std::string assembleShader(ShaderType type);
	std::string assembleDefines(ShaderType type);
	std::string runCompiler(ShaderType type, std::string inputFileName);
	std::vector<char> loadSPIR_V(std::string fileName);

};

#pragma once
