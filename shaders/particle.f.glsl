#version 330

layout(location = 0) out vec4 outputColor;
layout(location = 1) out vec4 glowColor;

in vec4 pcolor;

void main()
{
    outputColor = vec4(pcolor.xyz, 1);
    glowColor = vec4(pcolor.xyz, 1) * pcolor.a;
}
