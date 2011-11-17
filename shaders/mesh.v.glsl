#version 330

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 normal;
layout(location = 2) in vec2 texcoord;

uniform mat4 transform;

centroid out vec2 frag_texcoord;
centroid out vec4 frag_normal;

void main()
{
    gl_Position = transform * position;
    frag_texcoord = texcoord;
    frag_normal = normal;
}
