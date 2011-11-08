#version 330

out vec4 outputColor;
uniform sampler2D tex;
uniform vec3 color;
centroid in vec2 frag_texcoord;


void main()
{
    float mask = texture(tex, frag_texcoord).r;
    if (mask == 0.0)
        discard;
    outputColor = vec4(color, 1.0f);
}
