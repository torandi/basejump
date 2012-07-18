#version 330
#include "uniforms.glsl"

/* unmodified values */
out vec2 uv;
out vec4 color;
out vec3 orig_normal;
out vec3 orig_tangent;
out vec3 orig_bitangent;

/* multiplied values */
out vec3 mul_position;
out vec3 mul_normal;
out vec3 mul_tangent;
out vec3 mul_bitangent;

void main() {
   vec4 mul_pos = modelMatrix * in_position;
   gl_Position = projectionViewMatrix *  mul_pos;

   /* passthru */
   uv             = in_texcoord.st;
   color          = in_color;
   orig_normal    = in_normal.xyz;
   orig_tangent   = in_tangent.xyz;
   orig_bitangent = in_bitangent.xyz;

   /* multiplied */
   mul_normal     = (normalMatrix * in_normal).xyz;
   mul_tangent    = (normalMatrix * in_tangent).xyz;
   mul_bitangent  = (normalMatrix * in_bitangent).xyz;
}
