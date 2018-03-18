#pragma once

#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <assert.h>
#include <string>

/* The simple mesh read from 
*/
struct SimpleMesh
{
	enum BitFlag
	{
		NORMAL_BIT = 1,
		UV_BIT = 2,
		TRIANGLE_ARRAY = 4,		// Triangle draw array, no indices
		POS_4_COMPONENT = 8		// 4 floats per position component initiated with 1.
	};

	/* Part separator. */
	struct Part
	{
		uint32_t _ind;		//	Index to the indices list of the first model in the object.
		std::string _name;	//	Name of the object
	};
	uint32_t _mesh_flags;
	float _bb[6] = { FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX };
	//Face indices:
	std::vector<uint32_t> _face_ind;
	std::vector<uint32_t> _face_nor;
	std::vector<uint32_t> _face_uv;
	std::vector<Part> _part;			//Part separator
	//Vertex data:
	std::vector<float> _position;
	std::vector<float> _normal;
	std::vector<float> _uv;
	//Other:
	std::vector<unsigned int> _edges;

	SimpleMesh();

	/* If this mesh contains minimal set of required data. */
	bool valid();
	/* Append a mesh, concatening the data into this. */
	void append(SimpleMesh& other);

	/* Bake the mesh into a format suitable for graphics cards. Splitting vertex data into a triangle list. */
	void bake(unsigned int FLAG, SimpleMesh &bakeOutput);

	uint32_t size();
};

/* Read an .obj file. 
file	<<	File path
mesh	>>	The read file
return	>>	If the file could be opened/parsed successfully
*/
bool readObj(const char *file, SimpleMesh& mesh);

/* Fit BB with a vector. Stretching the box if it's outside the volume.  */
void fitBB(float* bb, float* v);
/* Join two bounding boxes. */
void mergeBB(float* bb, float* bb2);


#ifdef OBJ_READER_SIMPLE

#pragma region Doodad funcs

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

#pragma endregion

SimpleMesh::SimpleMesh()
	: _mesh_flags(0)
{
}

uint32_t SimpleMesh::size()
{
	return _face_ind.size();
}
bool SimpleMesh::valid()
{
	if (_position.size() == 0 || _face_ind.size() == 0) return false;
	for (int i = 0; i < _face_ind.size(); i++)
	{
		if (_face_ind[i] >= _position.size())
			return false;
	}
	for (int i = 0; i < _face_nor.size(); i++)
	{
		if (_face_nor[i] >= _normal.size())
			return false;
	}
	for (int i = 0; i < _face_uv.size(); i++)
	{
		if (_face_uv[i] >= _uv.size())
			return false;
	}
	for (int i = 0; i < _edges.size(); i++)
	{
		if (_edges[i] >= _position.size())
			return false;
	}
	return true;
}

void SimpleMesh::append(SimpleMesh& other)
{
	if (!other.valid()) return;
	//Reserve
	_face_ind.reserve(_face_ind.size() + other._face_ind.size());
	_face_nor.reserve(_face_nor.size() + other._face_nor.size());
	_face_uv.reserve(_face_uv.size() + other._face_uv.size());
	_position.reserve(_position.size() + other._position.size());
	_normal.reserve(_normal.size() + other._normal.size());
	_uv.reserve(_uv.size() + other._uv.size());
	_edges.reserve(_edges.size() + other._edges.size());
	// Copy indices
	size_t offset = _position.size() / 3;
	for (size_t i = 0; i < other._face_ind.size(); i++) _face_ind.push_back(other._face_ind[i] + offset);
	offset = _normal.size() / 3;
	for (size_t i = 0; i < other._face_nor.size(); i++) _face_nor.push_back(other._face_nor[i] + offset);
	offset = _uv.size() / 3;
	for (size_t i = 0; i < other._face_uv.size(); i++) _face_uv.push_back(other._face_uv[i] + offset);
	offset = _edges.size() / 3;
	for (size_t i = 0; i < other._edges.size(); i++) _edges.push_back(other._edges[i] + offset);
	// Copy vertices
	for (int i = 0; i < other._position.size(); i++) _position.push_back(other._position[i]);
	for (int i = 0; i < other._normal.size(); i++) _normal.push_back(other._normal[i]);
	for (int i = 0; i < other._uv.size(); i++) _uv.push_back(other._uv[i]);
	mergeBB(_bb, other._bb);
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
	if (COMP)
		bakeOutput._mesh_flags |= BitFlag::POS_4_COMPONENT;

	// Clear/Reserve
	bakeOutput._part.clear();
	pos.reserve(size());
	if (NOR)
		nor.reserve(size());
	if (UV)
		uv.reserve(size());
	// Copy bb
	for (uint32_t i = 0; i < 6; i++)
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
			if (!NO_IND)
				existMap[indID] = ind;
			// Append data
			pos.push_back(_position[indID._pInd*COMP]);
			pos.push_back(_position[indID._pInd * COMP + 1]);
			pos.push_back(_position[indID._pInd * COMP + 2]);
			if (COMP_4)
				pos.push_back(1);
			if (NOR)
			{
				nor.push_back(_normal[indID._nInd * 3]);
				nor.push_back(_normal[indID._nInd * 3 + 1]);
				nor.push_back(_normal[indID._nInd * 3 + 2]);
			}
			if (UV)
			{
				uv.push_back(_uv[indID._uvInd * 2]);
				uv.push_back(_uv[indID._uvInd * 2 + 1]);
			}
		}
		//Split objects
		if (sepInd < _part.size() && i == _part[sepInd]._ind)
		{
			bakeOutput._part.push_back({ (uint32_t)indices.size(), _part[sepInd]._name });
			sepInd++;
		}
		// Add indice
		if (!NO_IND)
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

void consumeWhiteSpace(std::stringstream& ss)
{
	char c;
	//Eat whitespace
	while ((c = ss.peek()) ==  ' ' || c == '\t')
		c = ss.get();
}
/* Fit BB with vector. */
void fitBB(float* bb, float* v)
{
	for (int i = 0; i < 3; i++) { bb[i * 2] = std::fminf(v[i], bb[i*2]); bb[i * 2 + 1] = std::fmaxf(v[i], bb[i * 2 + 1]); }
}
/* Fit bb with another bounding box bb2. */
void mergeBB(float* bb, float* bb2)
{
	for (int i = 0; i < 3; i++) { bb[i * 2] = std::fminf(bb2[i*2], bb[i * 2]); bb[i * 2 + 1] = std::fmaxf(bb2[i*2+1], bb[i * 2 + 1]); }
}

void initBB(float* bb, float* v)
{
	for (int i = 0; i < 3; i++) { bb[i*2] = v[i]; bb[i*2+1] = v[i]; }
}

/* Read nodes and faces from an obj file:
*/
 bool readObj(const char *file, SimpleMesh& mesh)
{
	//Open
	std::ifstream stream(file);
	if (stream.is_open())
	{
		//Line params:
		std::string s, head, item, vertex[3];
		std::stringstream ss, ss2;
		//Input vars:
		uint32_t a = 0, b = 0;
		uint32_t v[4], t[4], n[4];
		float xyz[3];
		//Read lines
		while (std::getline(stream, s))
		{
			ss = std::stringstream(s);
			if (!std::getline(ss, head, ' '))
				continue;
			if (head.empty()) continue;
			consumeWhiteSpace(ss);

			//Parse data:
			if (head.length() == 2)
			{
				if (head == "vn") {					//Normals
					ss >> xyz[0]; ss >> xyz[1]; ss >> xyz[2];
					mesh._normal.push_back(xyz[0]);
					mesh._normal.push_back(xyz[1]);
					mesh._normal.push_back(xyz[2]);
				}
				else if (head == "vt")				//UV
				{
					ss >> xyz[0]; ss >> xyz[1];
					mesh._uv.push_back(xyz[0]);
					mesh._uv.push_back(xyz[1]);
				}
			}
			else if (head.length() == 1)
			{
				if (head[0] == 'l') {				//Edges
					ss >> a; ss >> b;
					mesh._edges.push_back(a - 1);//.obj indices are +1
					mesh._edges.push_back(b - 1);
				}
				else if (head[0] == 'o')				//Parse object separation
				{
					SimpleMesh::Part m;
					ss >> m._name;
					m._ind = mesh.size();
					mesh._part.push_back(m);
				}
				else if (head[0] == 'v') {			//Vertices
					ss >> xyz[0]; ss >> xyz[1]; ss >> xyz[2];
					if (mesh._position.size() == 0) initBB(mesh._bb, xyz);
					else fitBB(mesh._bb, xyz);

					mesh._position.push_back(xyz[0]);
					mesh._position.push_back(xyz[1]);
					mesh._position.push_back(xyz[2]);
				}
				else if (head[0] == 'f')			//Faces
				{
					int num_vert = 0, num_tex = 0, num_norm = 0;
					while (std::getline(ss, item, ' '))
					{
						if (item.empty()) continue;
						int i = 0;
						ss2 = std::stringstream(item);
						for (; i < 3; i++)
						{
							if (!std::getline(ss2, vertex[i], '/'))
								break;
						}

						// Parse: Vertex/UV/Normal
						if (i > 0)
							v[num_vert++] = std::atoi(vertex[0].c_str());
						else
							break; //Terminate this line (face invalid)
						if (i > 1 && !vertex[1].empty())
							t[num_tex++] = std::atoi(vertex[1].c_str());
						if (i > 2 && !vertex[2].empty())
							n[num_norm++] = std::atoi(vertex[2].c_str());
					}

					if (num_vert <= 4)
					{
						for (int i = 0; i + 2 < num_vert; i++)
						{
							for (int ii = i; ii < i+3; ii++)
							{
								assert(v[ii] > 0);
								mesh._face_ind.push_back(v[ii] - 1);
								if (num_tex == num_vert) {
									assert(t[ii] > 0);
									mesh._face_uv.push_back(t[ii] - 1);
								}
								if (num_norm == num_vert) {
									assert(n[ii] > 0);
									mesh._face_nor.push_back(n[ii] - 1);
								}
							}
						}
					}
					else
						// TODO : Convert N-gons/quads to triangles
						std::cout << "Warning: N-gon detected only Triangles/Quads are supported\n";
				}
			}
		}
	}
	else //No nodemap could be read:
	{
		std::cout << "error - Node network file: " << file << " could not be opened\n";
		return false;
	}
	return true;
}

#endif