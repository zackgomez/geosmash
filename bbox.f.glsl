#version 330

layout(location = 0) out vec4 outputColor;
layout(location = 1) out vec4 glowColor;
uniform vec4 color;

void main()
{
    outputColor = vec4(color.rgb, 1.0f);
    glowColor = vec4(color.rgb * color.a, 1.0f);
}
