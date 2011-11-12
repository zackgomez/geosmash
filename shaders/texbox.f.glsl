#version 330

layout(location = 0) out vec4 outputColor;
layout(location = 1) out vec4 glowColor;
uniform sampler2D tex;
centroid in vec2 frag_texcoord;


void main()
{
    outputColor = texture(tex, frag_texcoord);
    glowColor = vec4(0.0f);
}
