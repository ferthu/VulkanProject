#version 450
layout(location=0) in vec4 position;
layout(location=1) in vec3 normal;

layout(set=0,binding=0) uniform Transform
{
	mat4 transform;
} t;

layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec4 worldPos;


void main()
{
	gl_Position = t.transform * position;
	out_normal = (t.transform * vec4(normal, 0.0)).xyz;
	worldPos = position;
}
