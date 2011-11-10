#version 330

layout(location = 0) out vec4 outputColor;
layout(location = 1) out vec4 glowColor;
uniform vec4 color;

void main()
{
    float alpha = color.a != 0 ? color.a : 1.0;
    outputColor = vec4(color.rgb, alpha);
    glowColor = vec4(color.rgb * color.a, 1.0f);
}
