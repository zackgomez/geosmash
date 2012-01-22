#version 330

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;

out vec4 pcolor;

void main()
{
    gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1);
    pcolor = color;
}
