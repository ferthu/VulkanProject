#include "Stuff/ObjReaderSimple.h"


#include<map>
/* Check if the flags are set in the property. */
inline bool hasFlag(uint32_t property, uint32_t flags)
{
	return (property & flags) == flags;
}
/* Find if the flags are equal. */
inline bool matchFlag(uint32_t property, uint32_t flags)
{
	return property == flags;
}
/* Unset bit in the flag. */
inline uint32_t rmvFlag(uint32_t property, uint32_t rmv)
{
	return property & ~rmv;
}

struct VerticeInd
{
	uint32_t _pInd, _nInd, _uvInd;

	bool operator<(const VerticeInd &o) const { return _pInd + _nInd + _uvInd < o._pInd + o._nInd + o._uvInd; }
};

/* Bake the mesh into a format suitable for graphics cards. Splitting vertex data into a triangle list. */
void SimpleMesh::bake(unsigned int FLAG, SimpleMesh &bakeOutput)
{
	std::map<VerticeInd, uint32_t> existMap;

	std::vector<uint32_t> indices;
	std::vector<float> pos;
	std::vector<float> nor;
	std::vector<float> uv;

	// Specified coordinates
	bool NOR = _normal.size() > 0 && hasFlag(FLAG, BitFlag::NORMAL_BIT);
	bool UV = _uv.size() > 0 && hasFlag(FLAG, BitFlag::UV_BIT);
	bool NO_IND = hasFlag(FLAG, BitFlag::TRIANGLE_ARRAY);
	bool COMP_4 = hasFlag(FLAG, BitFlag::POS_4_COMPONENT);
	float COMP = hasFlag(_mesh_flags, BitFlag::POS_4_COMPONENT) ? 4 : 3;
	if(COMP)
		bakeOutput._mesh_flags |= BitFlag::POS_4_COMPONENT;

	// Clear/Reserve
	bakeOutput._part.clear();
	pos.reserve(size());
	if(NOR)
		nor.reserve(size());
	if (UV)
		uv.reserve(size());
	// Copy bb
	for(uint32_t i = 0; i < 6; i++)
		bakeOutput._bb[i] = _bb[i];
	// Bake
	uint32_t sepInd = 0;
	for (size_t i = 0; i < size(); i++)
	{

		VerticeInd indID = { _face_ind[i], (NOR ? _face_nor[i] : 0u), (UV ? _face_uv[i] : 0u) };

		// Find vert indice
		auto it = NO_IND ? existMap.end() : existMap.find(indID);
		uint32_t ind;
		if (it != existMap.end())
			ind = it->second;
		else
		{
			// New vertice
			ind = (uint32_t)pos.size();
			if(!NO_IND)
				existMap[indID] = ind;
			// Append data
			pos.push_back(_position[indID._pInd*COMP]);
			pos.push_back(_position[indID._pInd * COMP + 1]);
			pos.push_back(_position[indID._pInd * COMP + 2]);
			if(COMP_4)
				pos.push_back(1);
			if (NOR)
			{
				nor.push_back(_normal[indID._nInd*3]);
				nor.push_back(_normal[indID._nInd *3+1]);
				nor.push_back(_normal[indID._nInd *3+2]);
			}
			if (UV)
			{
				uv.push_back(_uv[indID._uvInd*2]);
				uv.push_back(_uv[indID._uvInd *2+1]);
			}
		}
		//Split objects
		if (sepInd < _part.size() && i == _part[sepInd]._ind)
		{
			bakeOutput._part.push_back({ (uint32_t)indices.size(), _part[sepInd]._name });
			sepInd++;
		}
		// Add indice
		if(!NO_IND)
			indices.push_back(ind);
	}
	bakeOutput._face_ind = std::move(indices);
	bakeOutput._position = std::move(pos);
	bakeOutput._normal = std::move(nor);
	bakeOutput._uv = std::move(uv);
	// Clear the invalid lists
	bakeOutput._face_nor.clear();
	bakeOutput._face_uv.clear();
}