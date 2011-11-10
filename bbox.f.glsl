#version 330

layout(location = 0) out vec4 outputColor;
layout(location = 1) out vec4 glowColor;
uniform vec3 color;

void main()
{
    outputColor = vec4(color, 1.0f);
    glowColor = vec4(0.0, 0.0, 0.0, 0.0);
}
