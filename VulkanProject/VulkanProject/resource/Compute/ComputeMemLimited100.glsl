#version 450
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
layout(Rgba8, set=0, binding = 0) uniform image2D img_output;
layout(set=1, binding = 0) uniform UniformBufferObject 
{
  float locality;
} params;
layout(set=2, binding=0) uniform sampler2D myTex;

const int TOT_REG = 26; // Roughly it seems they are aligned.
const int N = TOT_REG - 13;
float arr[N];

void main() {
  // get index in global work group i.e x,y position
  vec2 pixel_coords = vec2(gl_GlobalInvocationID.xy) / 500;

  arr[0] = (2 + (gl_WorkGroupID.x + gl_WorkGroupID.y));
  arr[1] =  (gl_WorkGroupID.x + gl_WorkGroupID.y) / 100;
  for(uint i = 2; i < N; i++)
  {
    float sum = 0;
    for(uint ii = 0; ii < i-1; ii++)
    {
      sum += (arr[ii] / i) * 2 + texture(myTex, vec2(arr[ii], arr[ii])).r;
    }
    arr[i] = sqrt(sum) + texture(myTex, pixel_coords / params.locality + vec2(arr[i-1], arr[i-2])).r;
  }

  float valX = sin(arr[N-1]);
  float valY = valX;

  //for(uint i = 0; i < 50000; i++)
  vec4 pixel = texture(myTex, vec2(pixel_coords.x + valX, pixel_coords.y + valY));
  // output to a specific pixel in the image
  imageStore(img_output, ivec2(gl_GlobalInvocationID.xy), pixel);
}
