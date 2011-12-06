#version 330 

layout(location = 0) out vec4 outputColor;
layout(location = 1) out vec4 glowColor;
centroid in vec2 coord;

const float thresh = 0.9;
const float c = 100.;

const vec4 linecol = vec4(0.3, 0.05, 0.3, 1.f);

void main()
{
    //u += sin(t);
    //v += t;
    float u = coord.x;
    float v = coord.y;

    vec2 wave = vec2(sin(c * u), sin(c * v));
    wave = smoothstep(0.95, 1.00, wave);

    float fact = length(wave);
    
    if (fact == 0)
        discard;

    outputColor = fact * linecol;
    glowColor = vec4(linecol.rgb, 0.0f);
}

