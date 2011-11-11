#version 330

layout(location = 0) in vec4 position;

centroid out vec2 frag_texcoord;

void main()
{
    gl_Position = vec4(2*position.xyz, 1.0f);
    frag_texcoord = position.xy + vec2(0.5f);
}

