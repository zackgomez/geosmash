#version 120
const float pi = 3.1415926535;
const float r = 0.5;
const float length = 100.0;

attribute vec4 position;
uniform mat4 projectionMatrix, modelViewMatrix;
varying vec2 coord;

void main()
{
   // Convert from the [-5,5]x[-5,5] range provided into radians
   // between 0 and 2*pi
   coord = position.xy;

   // u -> [0, 2pi]
   float u = (position.x + 5.0) / 10.0 * 2 * pi;
   // v -> [-1, 1]
   float v = (position.y) / 5.0;

   vec3 world = vec3(r*cos(u), r*sin(u), 5*v);
   //world = 0.1*vec3(r*cos(u), r*sin(u), v);
   //world = 0.01*vec3(position.x, 0.f, position.y);

   gl_Position = projectionMatrix * modelViewMatrix * vec4(world,1.0);
}

