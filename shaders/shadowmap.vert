#version 150
#extension GL_ARB_explicit_attrib_location: enable

#include "uniforms.glsl"


layout (location = 0) in vec4 in_position;
layout (location = 1) in vec2 in_texcoord;
layout (location = 2) in vec4 in_normal;
layout (location = 3) in vec4 in_tangent;
layout (location = 4) in vec4 in_bitangent;
layout (location = 5) in vec4 in_color;

void main() {
   gl_Position = projectionViewMatrix *  modelMatrix * in_position;
}

