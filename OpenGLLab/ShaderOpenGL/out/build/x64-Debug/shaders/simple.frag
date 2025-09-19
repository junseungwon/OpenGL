#version 330 core
out vec4 FragColor;
uniform vec4 uColor; // CPU에서 넣어줄 '전역 상수' 같은 값

void main()
{
    FragColor = uColor;
}