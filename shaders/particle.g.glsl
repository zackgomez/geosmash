#version 120
#extension GL_EXT_geometry_shader4: enable

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;

varying in vec4 vpcolor[1];
varying in vec4 psize[1];
varying out vec4 pcolor;

void main(void)
{
    // TODO make this be a uniform or attribute
    const float r = 0.75f;
    for (int i = 0; i < gl_VerticesIn; i++)
    {
        float rx = psize[i].x/2;
        float ry = psize[i].y/2;

        gl_Position = gl_PositionIn[i] + vec4(rx, ry, 0, 0);
        gl_Position = projectionMatrix * modelViewMatrix * gl_Position;
        pcolor = vpcolor[i];
        EmitVertex();

        gl_Position = gl_PositionIn[i] + vec4(-rx, ry, 0, 0);
        gl_Position = projectionMatrix * modelViewMatrix * gl_Position;
        pcolor = vpcolor[i];
        EmitVertex();

        gl_Position = gl_PositionIn[i] + vec4(rx, -ry, 0, 0);
        gl_Position = projectionMatrix * modelViewMatrix * gl_Position;
        pcolor = vpcolor[i];
        EmitVertex();

        gl_Position = gl_PositionIn[i] + vec4(-rx, -ry, 0, 0);
        gl_Position = projectionMatrix * modelViewMatrix * gl_Position;
        pcolor = vpcolor[i];
        EmitVertex();
    }
    EndPrimitive();
}
