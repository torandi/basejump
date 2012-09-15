#version 330
#include "uniforms.glsl"

in vec2 uv;
out vec4 ocolor;

const float PixelWeight[13] = float[13](0.159577, 0.147308, 0.115877, 0.077674, 0.044368, 0.021596, 0.008958, 0.003166, 0.000954, 0.000245, 0.000054, 0.000010, 0.000002);

void main() {
  float dy = 1.0f / state.height;

  ocolor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	for ( int i = -12; i <= 12; i++ ){
		ocolor.rgb += texture(texture0, vec2(uv.x, clamp(uv.y + dy * i, 0, 1)) ).rgb * PixelWeight[abs(i)];
	}
}
