#version 120

uniform vec4 color;

varying vec2 frag_texcoord;

void main()
{
    gl_FragData[0] = vec4(color.rgb, 1.0f);

    vec2 edgedist = abs(frag_texcoord - 0.5);
    if (edgedist.x > 0.45 || edgedist.y > 0.45)
        gl_FragData[1] = vec4(color.rgb * color.a, 1.0f);
    else
        gl_FragData[1] = vec4(0.0f);
}
