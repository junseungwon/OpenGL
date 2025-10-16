#version 330 core
layout (location = 0) in vec3 aPos; // 위치 속성

void main()
{
    gl_Position = vec4(aPos, 1.0);
}