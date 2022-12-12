#version 150 core

out vec4 outColor;


uniform vec3 triangleColor;


void main()
{
    outColor  = vec4(triangleColor,1);
}
