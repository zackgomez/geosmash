#version 120

attribute vec4 position;
attribute vec4 normal;
attribute vec2 texcoord;

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;
uniform mat4 normalMatrix;

varying vec2 frag_texcoord;
varying vec4 frag_normal;
varying vec4 frag_pos;

void main()
{
    gl_Position = projectionMatrix * modelViewMatrix * position;

    frag_texcoord = texcoord;
    frag_normal = normalMatrix * vec4(normal.xyz, 0.f);
    frag_pos = modelViewMatrix * position;
}
