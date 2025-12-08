// shaders/debug_depth.fs
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
uniform sampler2D depthMap;

void main()
{
    float d = texture(depthMap, TexCoords).r;
    FragColor = vec4(vec3(d), 1.0);   // 회색조 시각화용 출력임
}
