#version 120 

varying vec2 coord;

const float thresh = 0.9;
const float c = 10.;
const float pi = 3.1415926535;

uniform vec3 color;
uniform float t;

//const vec3 linecol = vec3(0.3, 0.05, 0.3);
const vec3 cola = vec3(0.1, 0.05, 0.5);
const vec3 colb = vec3(0.1, 0.5, 0.1);
const vec3 colc = vec3(0.5, 0.1, 0.1);

float colfunc(float n)
{
    return (sin(n*t) + 1) / 2;
}

void main()
{
    //u += sin(t);
    //v += t;
    float u = coord.x;
    float v = coord.y / 5;
    v = -v;

    // TODO make the lines thicker the closer v is to 0
    float fact = smoothstep(0.90, 1.00, sin(c*pi*u));

    /*
    if (fact == 0)
        discard;
        */

    vec3 linecol = colfunc(1) * cola + colfunc(1/3.f) * colb + colfunc(1/5.f) * colc;

    gl_FragData[0] = vec4(fact * linecol, 1.f);
    gl_FragData[1] = vec4(0.f);//vec4(linecol.rgb, 0.1f);
}

