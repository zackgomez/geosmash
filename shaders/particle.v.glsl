#version 120

attribute vec3 position;
attribute vec4 color;

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;

varying vec4 vpcolor;

void main()
{
    gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1);
    vpcolor = color;
}
