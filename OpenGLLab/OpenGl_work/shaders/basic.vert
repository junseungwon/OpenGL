#version 330 core
layout (location = 0) in vec3 aPos;     // 위치
layout (location = 1) in vec2 aTexCoord; // 텍스처 좌표 (2D)

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

out vec2 TexCoord; // 프래그먼트 셰이더로 전달할 텍스처 좌표

void main()
{
    gl_Position = uProj * uView * uModel * vec4(aPos, 1.0);
    TexCoord = aTexCoord; // 텍스처 좌표 전달
}
