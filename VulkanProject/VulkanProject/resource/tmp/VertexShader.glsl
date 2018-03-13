#version 450
layout(location=0) in vec4 position;
layout(location=1) in vec3 normal;

layout(set=1,binding=1) uniform Transform
{
	mat4 transform;
} t;

layout(location = 0) out vec3 out_normal;


void main()
{
	vec4 vertexPos = position * t.transform;
	out_normal = (vec4(normal.xyz, 0.0) * t.transform).xyz;
}
