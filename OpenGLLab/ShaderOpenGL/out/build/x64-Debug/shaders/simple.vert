#version 330 core
layout (location = 0) in vec3 aPos; // C++에서 넘겨줄 정점 위치

void main()
{
    // 정점 위치를 '클립 공간'으로 출력해야 함
    gl_Position = vec4(aPos, 1.0);
}

