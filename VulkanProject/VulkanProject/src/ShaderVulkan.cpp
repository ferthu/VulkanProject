#include "ShaderVulkan.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <Windows.h>
#include <locale>
#include <codecvt>
#include "VulkanConstruct.h"
#include "UsefulFuncs.h"


ShaderVulkan::ShaderVulkan(const std::string & name, VulkanRenderer *renderHandle)
	: name(name), _renderHandle(renderHandle)
{
}

ShaderVulkan::~ShaderVulkan()
{
	destroyShaderObjects();
}

void ShaderVulkan::destroyShaderObjects()
{
	if (vertexShader)
		vkDestroyShaderModule(_renderHandle->getDevice(), vertexShader, nullptr);
	if (fragmentShader)
		vkDestroyShaderModule(_renderHandle->getDevice(), fragmentShader, nullptr);
}

void ShaderVulkan::setShader(const std::string & shaderFileName, ShaderType type)
{
	shaderFileNames[type] = shaderFileName;
}

int ShaderVulkan::compileMaterial(std::string & errString)
{
	//Clear first
	destroyShaderObjects();
	int success = createShaders();
	return success;
}

#pragma region Shader creation


int ShaderVulkan::createShaders()
{
	std::vector<char> vsData, fsData;

	if (ends_with(shaderFileNames[ShaderVulkan::ShaderType::VS], ".spv"))
		vsData = loadSPIR_V(shaderFileNames[ShaderVulkan::ShaderType::VS]);
	else
	{
		std::string tmpFile = assembleShader(ShaderVulkan::ShaderType::VS);
		std::string spvFile = runCompiler(ShaderVulkan::ShaderType::VS, tmpFile);
		vsData = loadSPIR_V(spvFile);
	}
	
	// 
	if (ends_with(shaderFileNames[ShaderVulkan::ShaderType::PS], ".spv"))
		fsData = loadSPIR_V(shaderFileNames[ShaderVulkan::ShaderType::PS]);
	else
	{
		std::string tmpFile = assembleShader(ShaderVulkan::ShaderType::PS);
		std::string spvFile = runCompiler(ShaderVulkan::ShaderType::PS, tmpFile);
		fsData = loadSPIR_V(spvFile);
	}

	VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.pNext = nullptr;
	shaderModuleCreateInfo.flags = 0;
	shaderModuleCreateInfo.codeSize = vsData.size();
	shaderModuleCreateInfo.pCode = reinterpret_cast<uint32_t*>(vsData.data());

	VkResult result = vkCreateShaderModule(_renderHandle->getDevice(), &shaderModuleCreateInfo, nullptr, &vertexShader);
	if (result != VK_SUCCESS)
	{
		std::cout << "Failed to create vertex shader module.\n";
		return -1;
	}
	shaderModuleCreateInfo.codeSize = fsData.size();
	shaderModuleCreateInfo.pCode = reinterpret_cast<uint32_t*>(fsData.data());

	result = vkCreateShaderModule(_renderHandle->getDevice(), &shaderModuleCreateInfo, nullptr, &fragmentShader);
	if (result != VK_SUCCESS)
	{
		std::cout << "Failed to create fragment shader module.\n";
		return -2;
	}
	return 0;
}

const char *path = "..\\assets\\Vulkan\\";
// Returns relative file path of created file
std::string ShaderVulkan::assembleShader(ShaderVulkan::ShaderType type)
{
	std::string fileName;

	if (type == ShaderVulkan::ShaderType::VS)
		fileName = "vertexShader.glsl.vert";
	else if (type == ShaderVulkan::ShaderType::PS)
		fileName = "fragmentShader.glsl.frag";
	else
		throw std::runtime_error("Unsupported shader type!");

	// Read shader into string
	std::ifstream file(shaderFileNames[type]);
	std::stringstream fileContents;
	if (file.is_open())
	{
		fileContents << file.rdbuf();
		file.close();
	}
	else
		throw std::runtime_error("Could not open shader file.");

	// Write complete shader into file
	std::ofstream completeShader(path + fileName);
	if (completeShader.is_open())
	{
		completeShader << "#version 450\n";

		completeShader << assembleDefines(type);

		completeShader << fileContents.str();

		completeShader.close();
	}
	else
		throw std::runtime_error("Could not create shader file.");

	return fileName;
}
// Returns relative file path of created file
std::string ShaderVulkan::assembleDefines(ShaderVulkan::ShaderType type)
{

	std::string args;
	for (std::string def : shaderDefines[type])
		args.append(def);
	return args;
}

void printThreadError(const char *msg)
{
	DWORD err = GetLastError();
	if (err != 0)
	{
		LPSTR messageBuffer = nullptr;
		size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

		std::string message(messageBuffer, size);

		//Free the buffer.
		LocalFree(messageBuffer);
		std::cout << msg << message << "\n";
	}
}
// Returns output file name
std::string ShaderVulkan::runCompiler(ShaderVulkan::ShaderType type, std::string inputFileName)
{
	// pass defines
	std::string commandLineStr;
	if (type == ShaderVulkan::ShaderType::VS)
		commandLineStr.append("-v -V -o vertexShader.spv -e main ");
	else if (type == ShaderVulkan::ShaderType::PS)
		commandLineStr.append("-v -V -o fragmentShader.spv -e main ");

	commandLineStr += "\"" + inputFileName + "\"";

	LPSTR commandLine = const_cast<char *>(commandLineStr.c_str());
	const char* lpDesk = "desktop";

	STARTUPINFOA startupInfo = {};
	startupInfo.cb = sizeof(STARTUPINFOA);
	startupInfo.lpReserved = NULL;
	startupInfo.lpDesktop = const_cast<char *>(lpDesk);
	startupInfo.lpTitle = NULL;
	startupInfo.dwX = 0;
	startupInfo.dwY = 0;
	startupInfo.dwXSize = 0;
	startupInfo.dwYSize = 0;
	startupInfo.dwXCountChars = 0;
	startupInfo.dwYCountChars = 0;
	startupInfo.dwFillAttribute = 0;
	startupInfo.dwFlags = 0;
	startupInfo.wShowWindow = 0;
	startupInfo.cbReserved2 = 0;
	startupInfo.lpReserved2 = NULL;
	startupInfo.hStdInput = 0;
	startupInfo.hStdError = 0;
	startupInfo.hStdOutput = 0;

	LPSTARTUPINFOA startupInfoPointer = &startupInfo;

	PROCESS_INFORMATION processInfo = {};
	if (!CreateProcessA("..\\assets\\Vulkan\\glslangValidator.exe", commandLine, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, path, startupInfoPointer, &processInfo))
	{
		//HRESULT res = HRESULT_FROM_WIN32(GetLastError());
		std::cout << "Failed to start shader compilation process. Ensure '..\\assets\\Vulkan\\glslangValidator.exe' exists.\n";
		throw std::runtime_error("Failed to start shader compilation process.");
	}

	WaitForSingleObject(processInfo.hProcess, INFINITE);


	DWORD exitCode;
	bool acquired = GetExitCodeProcess(processInfo.hProcess, &exitCode);

	if (!acquired)
	{
		printThreadError("Error: Fetching process error failed with msg: ");
		throw std::runtime_error("Could not get exit code from process.");
	}
	else
		std::cout << "Process exited with msg: " << exitCode << "\n";

	CloseHandle(processInfo.hProcess);
	CloseHandle(processInfo.hThread);


	return (type == ShaderVulkan::ShaderType::VS) ? "..\\assets\\Vulkan\\vertexShader.spv" : "..\\assets\\Vulkan\\fragmentShader.spv";
}

std::vector<char> ShaderVulkan::loadSPIR_V(std::string fileName)
{
	// Open file and seek to end
	std::ifstream shaderFile(fileName, std::ios::ate | std::ios::binary);

	if (!shaderFile.is_open())
		throw std::runtime_error("Could not open SPIR-V file.");

	// Get file size
	size_t fileSize = static_cast<size_t>(shaderFile.tellg());

	// Create and resize vector to fit the file
	std::vector<char> data;
	data.resize(fileSize);

	// Reset to beginning
	shaderFile.seekg(0);

	// Read data into vector
	shaderFile.read(data.data(), fileSize);
	shaderFile.close();

	return data;
}

#pragma endregion
