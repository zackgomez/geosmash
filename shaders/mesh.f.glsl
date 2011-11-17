#version 330

layout(location = 0) out vec4 outputColor;
layout(location = 1) out vec4 glowColor;
centroid in vec2 frag_texcoord;
centroid in vec4 frag_normal;

uniform vec4 color;


void main()
{
    outputColor = vec4(frag_texcoord, 0, 1);
    glowColor = vec4(0.0f);
}
