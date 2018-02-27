layout(local_size_x = 1, local_size_y = 1) in;
layout(set=0, binding=0) uniform sampler2D myTex;
layout(Rgba8, set=1, binding = 0) uniform image2D img_output;

void main() {
  // get index in global work group i.e x,y position
  ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
  uvec4 pixel = uvec4(texelFetch(myTex, pixel_coords, 0));

  // output to a specific pixel in the image
  imageStore(img_output, pixel_coords, pixel);
}
