#version 330 core
out vec4 FragColor;
in vec2 vUV;
uniform sampler2D uTex0;
uniform sampler2D uTex1;
uniform float uMix;
void main(){
    vec4 c0 = texture(uTex0, vUV);
    vec4 c1 = texture(uTex1, vUV);
    FragColor = mix(c0, c1, uMix);
}

