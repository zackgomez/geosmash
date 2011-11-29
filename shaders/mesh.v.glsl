#version 330

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 normal;
layout(location = 2) in vec2 texcoord;

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;
uniform mat4 normalMatrix;
uniform vec4 lightPos;

centroid out vec2 frag_texcoord;
centroid out vec4 frag_normal;
centroid out vec4 frag_pos;

void main()
{
    gl_Position = projectionMatrix * modelViewMatrix * position;
    frag_texcoord = texcoord;
    frag_normal = normalMatrix * vec4(normal.xyz, 0.f);
    frag_pos = modelViewMatrix * position;
}
