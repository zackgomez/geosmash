#version 120
const float pi = 3.1415926535;
const float r = 0.5;

uniform float t;

attribute vec4 position;
uniform mat4 projectionMatrix, modelViewMatrix;
varying vec2 coord;

void main()
{
   // Convert from the [-5,5]x[-5,5] range provided into radians
   // between 0 and 2*pi
   coord = position.xy;

   float u = (coord.x + 5) / 10 * 2 * pi;
   float v = (coord.y + 5) / 10 * 6 * pi;

   // wobbly sphere
   vec3 world = vec3(
    sin(u) * sin(v) + 0.05 * sin(t) * cos(4 * v),
    cos(v),
    cos(u) * sin(v) + 0.05 * sin(t) * cos(4 * u));

   /*
   vec3 world = vec3(
       sin(u+t) * (cos(v) + 2),
       v - 3 + 0.4 * sin(2 * t),
       cos(u+t) * (cos(v) + 2));
       */

   //world.y *= 2;

   gl_Position = projectionMatrix * modelViewMatrix * vec4(world, 1.0);





   /*
   float u = (position.x + 5.0) / 10.0 * 2 * pi;
   float v = (position.y + 5.0) / 10.0 * 2 * pi;

   vec3 world = vec3(r*cos(u)*sin(v), r*cos(v), r*sin(u)*sin(v));
   //world = 0.01*vec3(position.x, 0.f, position.y);

   gl_Position = projectionMatrix * modelViewMatrix * vec4(world,1.0);
   */
}

