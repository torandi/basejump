#version 330
#include "uniforms.glsl"

in vec2 uv;
out vec4 ocolor;

const float PixelWeight[13] = float[13](0.159577, 0.147308, 0.115877, 0.077674, 0.044368, 0.021596, 0.008958, 0.003166, 0.000954, 0.000245, 0.000054, 0.000010, 0.000002);

void main() {
  float dx = 1.0f / resolution.width;

  ocolor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	for ( int i = -12; i <= 12; i++ ){
		ocolor.rgb += texture(texture0, vec2(clamp(uv.x + dx * i, 0, 1), uv.y) ).rgb * PixelWeight[abs(i)];
	}
}
