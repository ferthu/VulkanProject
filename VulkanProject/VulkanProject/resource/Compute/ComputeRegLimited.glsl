#version 450
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
layout(Rgba8, set=0, binding = 0) uniform image2D img_output;

const int TOT_REG = 53; // Roughly it seems they are aligned.
const int N = TOT_REG - 10;
float arr[N];

void main() {
  // get index in global work group i.e x,y position
  ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);

  arr[0] = (2 + gl_LocalInvocationID.x + gl_LocalInvocationID.y);
  arr[1] = 0.1f;
  for(uint i = 2; i < N; i++)
  {
    float sum = 0;
    for(uint ii = 0; ii < i-1; ii++)
    {
      sum += arr[ii] * 2 / i;
    }
    arr[i] = sqrt(sum * arr[i-1]);
  }
  arr[N-1] /= 100.f;

  for(uint i = 0; i < 50000; i++)
	arr[N-1] = cos(sin(arr[N-1]));

  vec4 pixel = vec4(arr[N-1], arr[N-1], arr[N-1], 1.f);
  // output to a specific pixel in the image
  imageStore(img_output, pixel_coords, pixel);
}
