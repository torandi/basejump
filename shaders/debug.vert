#version 150
#include "uniforms.glsl"

out VertexData {
	vec3 normal;
	vec3 tangent;
	vec3 bitangent;
} vertexData;

void main() {
	gl_Position  = modelMatrix * in_position;
	vertexData.normal = (normalMatrix * in_normal).xyz;
	vertexData.tangent = (normalMatrix * in_tangent).xyz;
	vertexData.bitangent = (normalMatrix * in_bitangent).xyz;
}
