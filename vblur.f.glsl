#version 330

out vec4 outputColor;

uniform sampler2D tex;
centroid in vec2 frag_texcoord;
uniform vec2 texsize;

uniform float offset[3] = float[]( 0.0, 1.3846153846, 3.2307692308 );
uniform float weight[3] = float[]( 0.5, 0.4162162162, 0.1002702703 );


void main()
{
    vec4 color = texture2D(tex, frag_texcoord) * weight[0];
    for (int i=1; i<3; i++) {
        color +=
            texture2D(tex, frag_texcoord + vec2(0.0, offset[i])  / texsize.y)
            * weight[i];
        color +=
            texture2D(tex, frag_texcoord - vec2(0.0, offset[i]) / texsize.y)
            * weight[i];
    }
    outputColor = color;
}
