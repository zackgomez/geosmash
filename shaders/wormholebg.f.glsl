#version 120 

varying vec2 coord;

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

    gl_FragData[0] = fact * linecol;
    gl_FragData[1] = vec4(linecol.rgb, 0.0f);
}

