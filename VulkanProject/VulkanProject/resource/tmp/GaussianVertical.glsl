#version 450
layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;
layout(Rgba8, set=0, binding = 0) uniform image2D img;
const uint IMG_DIM = 1024;

//const uint H_DIM = 5;
//float conv[H_DIM+1] = {0.39905, 0.242036, 0.054, 0.004433};

const uint H_DIM = 4;
float conv[H_DIM+1] = {0.266559, 0.213444, 0.109586, 0.036074, 0.007614};

shared vec3 s_mem[gl_WorkGroupSize.x + 2 * H_DIM];

uint divCeil(uint numer, uint denom)
{
  //numer >= 0 && (numer + denom < OVERFLOW)
  return (numer + denom - 1) / denom;
}
//Index for the shared array
uint sharedInd() { return gl_LocalInvocationID.x + H_DIM; }

void fetchData(uint index)
{
    // Fetch data (interval + overlap end)
    s_mem[sharedInd()].bgr = imageLoad(img, ivec2(gl_WorkGroupID.y, index)).rgb;
    if(gl_LocalInvocationID.x < H_DIM)
      s_mem[sharedInd() + gl_WorkGroupSize.x].bgr =
        imageLoad(img, ivec2(gl_WorkGroupID.y, index + gl_WorkGroupSize.x)).rgb;
}

uint calcIndex(uint iter)
{
  return gl_LocalInvocationID.x + gl_WorkGroupSize.x * iter;
}

void main() {

  uint s_ind = sharedInd();
  uint iters = divCeil(IMG_DIM, gl_WorkGroupSize.x);
  for(uint i = 0; i < iters; i++)
  {
    // Fetch data
    uint index =  calcIndex(i);
    fetchData(index);
    if(i == 0 && gl_LocalInvocationID.x < H_DIM) // first iter is out of bounds!..
      s_mem[gl_LocalInvocationID.x] = s_mem[H_DIM];
    // Sync. memory access
    memoryBarrierShared();
    barrier();

    // Blur
    vec3 sum = conv[0] * s_mem[s_ind];
    for(uint ii = 1; ii < H_DIM + 1; ii++)
      sum += conv[ii] * (s_mem[s_ind + ii] + s_mem[s_ind - ii]);
    // Output result
    imageStore(img, ivec2(gl_WorkGroupID.y, index), vec4(sum.rgb, 1.f));
    barrier(); // Sync. before loading next
    if(gl_LocalInvocationID.x < H_DIM) // initial overlap
      s_mem[gl_LocalInvocationID.x] = s_mem[s_ind + gl_WorkGroupSize.x];
    barrier();
  }
}
