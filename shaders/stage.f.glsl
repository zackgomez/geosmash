#version 120

varying vec2 frag_texcoord;
varying vec4 frag_normal;
varying vec4 frag_pos;

uniform vec4 color;
uniform vec4 lightpos;

void main()
{
    vec3 normal = normalize(vec3(frag_normal));
    vec3 lightdir = normalize(vec3(lightpos - frag_pos));
    float ndotl = dot(normal, lightdir);
    ndotl = min(max(ndotl, 0) + 0.3, 1);

    gl_FragData[0] = vec4(ndotl * vec3(color), 1.0f);
    gl_FragData[1] = vec4(color.rgb, 1.f) * color.a;
}
