#version 330 core // GLSL 버전 지정 (OpenGL 3.3 코어 프로파일)

// 입력 변수: VBO에서 오는 정점 데이터
// layout (location = X)는 해당 속성이 파이프라인의 몇 번 슬롯에 연결되는지를 나타냅니다.
layout (location = 0) in vec3 aPos;       // 정점 위치 (로컬 공간)
layout (location = 1) in vec3 aNormal;    // 정점 법선 벡터
layout (location = 2) in vec2 aTexCoords; // 텍스처 좌표

// 출력 변수: 프래그먼트 셰이더로 전달될 값 (보간되어 전달됩니다)
out vec3 FragPos;   // 월드 공간에서의 프래그먼트 위치
out vec3 Normal;    // 월드 공간에서의 법선 벡터
out vec2 TexCoords; // 텍스처 좌표

// 유니폼 변수: CPU에서 GPU로 데이터를 전달하는 데 사용되는 전역 변수
uniform mat4 model;      // 모델 변환 행렬
uniform mat4 view;       // 뷰 변환 행렬 (카메라)
uniform mat4 projection; // 투영 변환 행렬

// 셰이더 프로그램의 메인 함수 (진입점)
void main()
{
    // 1. gl_Position 계산: 최종 클립 공간의 정점 위치
    // model, view, projection 행렬을 순서대로 곱하여 정점 위치를 변환합니다.
    gl_Position = projection * view * model * vec4(aPos, 1.0);

    // 2. FragPos 계산: 월드 공간에서의 프래그먼트 위치
    // 광원 계산을 위해 프래그먼트의 월드 공간 위치가 필요합니다.
    FragPos = vec3(model * vec4(aPos, 1.0));

    // 3. Normal 계산: 월드 공간에서의 법선 벡터
    // 법선 벡터는 위치와 달리 스케일 변환의 영향을 다르게 받으므로
    // 정확히는 `mat3(transpose(inverse(model))) * aNormal`을 사용하지만,
    // 여기서는 간단하게 `mat3(model)`을 사용합니다.
    Normal = mat3(model) * aNormal;

    // 4. TexCoords 전달: 텍스처 좌표
    // 프래그먼트 셰이더로 텍스처 좌표를 그대로 전달합니다.
    TexCoords = aTexCoords;
}