#version 330

out vec4 outputColor;

uniform sampler2D tex;
centroid in vec2 frag_texcoord;
uniform vec2 texsize;

uniform float offset[3] = float[]( 0.0, 1.3846153846, 3.2307692308 );
uniform float weight[3] = float[](5.f / 10, 4.f / 15, 3.f / 15);


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
    outputColor = color;
}
