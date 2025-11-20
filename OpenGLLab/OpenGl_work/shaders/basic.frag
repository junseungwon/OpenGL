#version 330 core
out vec4 FragColor;

in vec2 TexCoord;          // vertex shader에서 전달받은 텍스처 좌표
uniform sampler2D texture1; // C++에서 바인딩한 텍스처

void main()
{
    FragColor = texture(texture1, TexCoord);
}
