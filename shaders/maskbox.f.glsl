#version 330

layout(location = 0) out vec4 outputColor;
layout(location = 1) out vec4 glowColor;
uniform sampler2D tex;
uniform vec4 color;
uniform vec2 texsize;
centroid in vec2 frag_texcoord;


void main()
{
    float mask = texture(tex, frag_texcoord).r;
    vec2 bixeldist = abs(fract(frag_texcoord * texsize) - 0.5);

    vec2 fact = smoothstep(0.35, 0.40, bixeldist);
    if (mask == 0.0)
        discard;
    if (fact.x == 0 && fact.y == 0)
        discard;

    outputColor = vec4(color.rgb, length(fact) * 1.0f);
    glowColor = vec4(color.rgb * color.a, 1.0f);
}
