#version 120

attribute vec3 position;
attribute vec3 velocity;
attribute vec3 size;
attribute vec4 color;
attribute float lifetime;

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;

varying vec4 vpcolor;
varying vec3 psize;

void main()
{
    gl_Position = vec4(position, 1);
    vpcolor = color;
    psize = size;
}
