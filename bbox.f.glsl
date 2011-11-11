#version 330

layout(location = 0) out vec4 outputColor;
layout(location = 1) out vec4 glowColor;
uniform vec4 color;
centroid in vec2 frag_texcoord;

void main()
{
    outputColor = vec4(color.rgb, 1.0f);
    vec2 edgedist = abs(frag_texcoord - 0.5);
    if (edgedist.x > 0.45 || edgedist.y > 0.45)
        glowColor = vec4(color.rgb * color.a, 1.0f);
    else
        glowColor = vec4(0.0f);
}
