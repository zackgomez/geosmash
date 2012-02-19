#version 120 

varying vec2 coord;

const float thresh = 0.9;
const float c = 10.;
const float pi = 3.1415926535;

const vec3 linecol = vec3(0.3, 0.05, 0.3);

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

    gl_FragData[0] = vec4(fact * linecol, 1.f);
    gl_FragData[1] = vec4(0.f);//vec4(linecol.rgb, 0.0f);
}

