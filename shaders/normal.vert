#version 330
#include "uniforms.glsl"

out vec3 position;
out vec3 normal;
out vec3 tangent;
out vec3 bitangent;
out vec2 texcoord;

void main() {
   vec4 w_pos = modelMatrix * in_position;
   gl_Position = projectionViewMatrix *  w_pos;

   position = w_pos.xyz;
   texcoord = in_texcoord.st;
   normal = (normalMatrix * in_normal).xyz;
   tangent = (normalMatrix * in_tangent).xyz;
   bitangent = (normalMatrix * in_bitangent).xyz;
}
