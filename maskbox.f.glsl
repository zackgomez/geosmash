#version 330

layout(location = 0) out vec4 outputColor;
layout(location = 1) out vec4 glowColor;
uniform sampler2D tex;
uniform vec4 color;
centroid in vec2 frag_texcoord;


void main()
{
    float mask = texture(tex, frag_texcoord).r;
    if (mask == 0.0)
        discard;
    outputColor = vec4(color.rgb, 1.0f);
    glowColor = vec4(color.rgb * color.a, 1.0f);
}
