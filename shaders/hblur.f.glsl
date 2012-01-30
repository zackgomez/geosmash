#version 120

uniform sampler2D tex;
uniform vec2 texsize;

varying vec2 frag_texcoord;

uniform float offset[3] = float[]( 0.0, 1.3846153846, 3.2307692308 );
uniform float weight[3] = float[](5.f / 10, 4.5f / 15, 3.f / 15);

void main()
{
    vec4 color = texture2D(tex, frag_texcoord) * weight[0];
    for (int i=1; i<3; i++) {
        color +=
            texture2D(tex, frag_texcoord + (vec2(offset[i], 0.0) / texsize.x))
            * weight[i];
        color +=
            texture2D(tex, frag_texcoord - (vec2(offset[i], 0.0) / texsize.x))
            * weight[i];
    }
    gl_FragData[0] = color;
}
