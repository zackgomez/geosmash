#version 120

attribute vec4 position;

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;

varying vec2 frag_texcoord;

void main()
{
    gl_Position = projectionMatrix * modelViewMatrix * position;
    frag_texcoord = position.xy + vec2(0.5f);
}
