#version 330

layout(location = 0) out vec4 outputColor;
layout(location = 1) out vec4 glowColor;
centroid in vec2 frag_texcoord;
centroid in vec4 frag_normal;
centroid in vec4 frag_pos;

uniform vec4 color;
uniform vec4 lightpos;


void main()
{
    vec3 normal = normalize(vec3(frag_normal));
    vec3 lightdir = normalize(vec3(lightpos - frag_pos));
    float ndotl = dot(normal, lightdir);
    ndotl = min(max(ndotl, 0) + 0.3, 1);

    outputColor = vec4(ndotl * vec3(color), 1.0f);
    glowColor = vec4(0.0f, 0.0f, 0.0f, 1.f);
}
