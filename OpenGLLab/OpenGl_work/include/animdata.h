/*
 * animdata.h
 * 
 * 스켈레탈 애니메이션에 필요한 데이터 구조체들을 정의합니다.
 * 
 * 스켈레탈 애니메이션이란?
 * - 캐릭터 내부에 뼈대(Skeleton)를 넣고, 뼈대를 움직여서 애니메이션을 만드는 방식입니다.
 * - 사람이 걷거나 뛰는 것처럼 자연스러운 동작을 만들 수 있습니다.
 * - 뼈대가 움직이면 주변 정점(피부)도 따라서 움직입니다.
 */

#pragma once

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <string>
#include <vector>

// 셰이더에서 처리할 수 있는 최대 뼈대 개수
const int MAX_BONES = 100;

/*
 * BoneInfo - 뼈대 정보 구조체
 * 
 * 각 뼈대의 ID와 오프셋 행렬을 저장합니다.
 * 
 * - id: 뼈대의 고유 번호 (0, 1, 2, ...)
 * - offset: 오프셋 행렬 (정점을 뼈대의 로컬 공간으로 변환)
 *           모델의 원래 자세에서 뼈대의 위치를 기준으로 정점 위치를 변환합니다.
 */
struct BoneInfo
{
    int id;            // 뼈대 고유 번호
    glm::mat4 offset;  // 오프셋 행렬
};

/*
 * AssimpNodeData - 노드 계층 구조 데이터
 * 
 * 3D 모델은 트리 구조로 되어 있습니다.
 * 예: 어깨 → 팔 → 손 → 손가락
 * 부모 노드가 움직이면 자식 노드도 따라 움직입니다.
 * 
 * - transformation: 이 노드의 변환 행렬
 * - name: 노드 이름
 * - childrenCount: 자식 노드 개수
 * - children: 자식 노드들
 */
struct AssimpNodeData
{
    glm::mat4 transformation;           // 변환 행렬 (위치, 회전, 크기)
    std::string name;                   // 노드 이름
    int childrenCount;                  // 자식 개수
    std::vector<AssimpNodeData> children;  // 자식 노드들
};
