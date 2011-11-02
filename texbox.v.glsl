#version 330

layout(location = 0) in vec4 position;

uniform mat4 transform;
uniform sampler2D texture;

centroid out vec2 frag_texcoord;

void main()
{
    gl_Position = transform * position;
    frag_texcoord = position.xy + vec2(0.5f);
}
