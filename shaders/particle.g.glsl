#version 120
#extension GL_EXT_geometry_shader4: enable

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;

varying in vec4 vpcolor[1];
varying out vec4 pcolor;

void main(void)
{
    for (int i = 0; i < gl_VerticesIn; i++)
    {
        gl_Position = gl_PositionIn[i] + vec4(-0.5, 0.5, 0, 0);
        pcolor = vpcolor[i];
        EmitVertex();

        gl_Position = gl_PositionIn[i] + vec4(0.5, 0.5, 0, 0);
        pcolor = vpcolor[i];
        EmitVertex();

        gl_Position = gl_PositionIn[i] + vec4(0.5, -0.5, 0, 0);
        pcolor = vpcolor[i];
        EmitVertex();

        gl_Position = gl_PositionIn[i] + vec4(-0.5, -0.5, 0, 0);
        pcolor = vpcolor[i];
        EmitVertex();
    }
    EndPrimitive();
}
