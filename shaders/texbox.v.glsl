#version 330

layout(location = 0) in vec4 position;

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;

centroid out vec2 frag_texcoord;

void main()
{
    gl_Position = projectionMatrix * modelViewMatrix * position;
    frag_texcoord = position.xy + vec2(0.5f);
}
