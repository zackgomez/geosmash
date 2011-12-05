#version 330
const float pi = 3.1415926535;
const float r = 0.5;

layout(location = 0) in vec4 position;
uniform mat4 projectionMatrix, modelViewMatrix;
centroid out vec2 coord;

void main()
{
   // Convert from the [-5,5]x[-5,5] range provided into radians
   // between 0 and 2*pi
   coord = position.xy;

   float u = (position.x + 5.0) / 10.0 * 2 * pi;
   float v = (position.y + 5.0) / 10.0 * 2 * pi;

   vec3 world = vec3(r*cos(u)*sin(v), r*cos(v), r*sin(u)*sin(v));
   //world = 0.01*vec3(position.x, 0.f, position.y);

   gl_Position = projectionMatrix * modelViewMatrix * vec4(world,1.0);
}
