#version 450
struct Particle
{
  float x;
	vec2 pos;
	vec2 vel;
};

const int complexity = 1000;
// Binding 0 : Position storage buffer
layout(std140, binding = 0) buffer Pos
{
   Particle particles[ ];
};

layout (local_size_x = 256) in;

void main()
{
    // Thread ID
    uint index = gl_GlobalInvocationID.x;

    // Random computation
    float x = particles[index].x;
    vec2 pos = particles[index].pos;
    for(int i = 0; i < complexity; i++)
    {
      x = mod(x + 0.001f, 2.f*3.14159f);
      pos += vec2(0.1,0.2) * sin(x);
    }
    particles[index].pos = pos;
    particles[index].x = x;

  }
