#version 330

out vec4 outputColor;
uniform sampler2D tex;
centroid in vec2 frag_texcoord;


void main()
{
    outputColor = texture(tex, frag_texcoord);
}
