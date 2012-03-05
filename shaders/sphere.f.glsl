#version 120 

varying vec2 coord;
uniform float t;
const float pi = 3.1415926;

const float thresh = 0.9;
const float c = 30.;

vec4 linecol = vec4(0.3, 0.05, 0.3, 1.f);
vec4 cola = vec4(0.3, 0, 0, 1.f);
vec4 colb = vec4(0, 0.3, 0, 1.f);
vec4 colc = vec4(0, 0, 0.3, 1.f);

void main()
{
    float u = coord.x;
    float v = coord.y;
    u += t/40;
    //v += t/40;

    float colt = t/5;
    linecol = cola * abs(sin(colt)) + colb * abs(sin(colt + 2*pi/3)) + colc*abs(sin(colt + 4*pi/3));

    vec2 wave = vec2(sin(c * u), sin(c * v));
    wave = smoothstep(0.95, 1.00, wave);

    float fact = length(wave);
    
    if (fact == 0)
        discard;

    gl_FragData[0] = fact * linecol;
    gl_FragData[1] = vec4(linecol.rgb, 0.0f);
}

