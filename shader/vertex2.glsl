#version 150 core

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;
uniform mat4 rotationMatrix;

uniform vec3 triangleColor;


in vec3 position;


// out vec3 color;
out vec4 pos;


void main()
{
    // color = triangleColor;
    // pos = projMatrix * viewMatrix * modelMatrix * rotationMatrix * vec4(position, 1.0);
    pos = rotationMatrix * vec4(position, 1.0); 
    gl_Position = projMatrix * viewMatrix * modelMatrix * rotationMatrix * vec4(position, 1.0);
}