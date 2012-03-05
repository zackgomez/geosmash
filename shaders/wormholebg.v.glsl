#version 120
const float pi = 3.1415926535;
const float r = 0.6;
const float length = 100.0;

attribute vec4 position;
uniform mat4 projectionMatrix, modelViewMatrix;
uniform float t;
varying vec2 coord;

void main()
{
   // Save for the frag shader
   // coord is [-5, 5] x [-5, 5]
   coord = position.xy;

   // u -> [0, 2pi]
   float u = (position.x + 5.0) / 10.0 * 2 * pi;
   // v -> [-1, 1]
   float v = (position.y) / 5.0;
   v *= 2;

   float off = 0.2 * sin(5*v - t) * 5*(-v);
   float xfact = (sin(pi*t/10 + v) + 1) / 2;

   float xoff = off * xfact;
   float yoff = off * (1 - xfact);

   float x = r*(cos(u)+.05*cos(t)*cos(10*u)) * (sin(t/2) + 3)/1.5 + xoff;
   float y = r*(sin(u)+.05*cos(t)*cos(10*u)) * (cos(t/5) + 3)/1.5 + yoff;

   //x *= 1.2 + v;
   //y *= 1.2 + v;
   float z = 2*v;

   vec3 world = vec3(x, y, z);
   //world = 0.1*vec3(r*cos(u), r*sin(u), v);
   //world = 0.01*vec3(position.x, 0.f, position.y);

   gl_Position = projectionMatrix * modelViewMatrix * vec4(world, 1.0);
}

