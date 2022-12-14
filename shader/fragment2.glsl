#version 150 core

out vec4 outColor;
in vec4 pos;


uniform vec3 triangleColor;


void main()
{
    outColor  =  (vec4(1, 0, 0, 1) * abs(1- pos.y) )+ (vec4(0, 0, 1, 1) * abs(pos.y) );
}
