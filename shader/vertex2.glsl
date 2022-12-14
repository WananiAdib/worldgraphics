#version 150 core

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;
uniform mat4 rotationMatrix;

uniform vec3 triangleColor;


in vec3 position;


out vec3 color;
out vec3 ;


void main()
{
    color = triangleColor;
    // pos = vec3(modelMatrix * vec4(position, 1.0));
    gl_Position = projMatrix * viewMatrix * modelMatrix * rotationMatrix * vec4(position, 1.0);
}