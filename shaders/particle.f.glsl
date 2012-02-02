#version 120

varying vec4 pcolor;

void main()
{
    gl_FragData[0] = vec4(pcolor.xyz, 1);
    gl_FragData[1] = vec4(pcolor.xyz, 1) * pcolor.a;
}
