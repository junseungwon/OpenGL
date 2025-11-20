#version 330 core
out vec4 FragColor;

void main()
{
    // 광원 큐브는 항상 밝게 (흰색)
    FragColor = vec4(1.0);
}
