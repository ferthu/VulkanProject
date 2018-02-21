#pragma once
#include <unordered_map>
#include <gl\glew.h>
#include "../Mesh.h"
#include <vulkan/vulkan.h>

class MeshVulkan :
	public Mesh
{
public:
	MeshVulkan();
	~MeshVulkan();


	//virtual void setTechnique(Technique *technique);
	//virtual void setTxBuffer(ConstantBuffer *cb);

};
