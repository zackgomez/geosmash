#version 330

layout(location = 0) in vec4 position;
uniform mat4 transform;
uniform vec3 color;

mat4 make_ortho(float l, float r, float b, float t, float n, float f)
{
    return mat4(
        vec4(2.0 / (r - l), 0.0, 0.0, 0.0),
        vec4(0.0, 2.0 / (t - b), 0.0, 0.0),
        vec4(0.0, 0.0, -2.0 / (f - n), 0.0),
        vec4(-(r + l)/ (r - l), -(t + b) / (t - b), - (f + n) / (f - n), 1.0));
}

void main()
{
    gl_Position = make_ortho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0) * transform * position;
}
