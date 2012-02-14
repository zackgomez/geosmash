#version 120

varying vec4 pcolor;

void main()
{
    gl_FragData[0] = vec4(pcolor.rgb, 1);
    gl_FragData[1] = vec4(pcolor.rgb, 1) * pcolor.a;
}
