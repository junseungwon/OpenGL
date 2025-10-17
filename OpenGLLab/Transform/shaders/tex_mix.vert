#version 330 core
layout (location=0) in vec3 aPos;
layout (location=1) in vec3 aColor; // 예시용(미사용)
layout (location=2) in vec2 aUV;
out vec2 vUV;
void main(){
    gl_Position = vec4(aPos, 1.0);
    vUV = aUV;
}

