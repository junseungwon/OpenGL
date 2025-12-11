/*
 * bone.h
 * 
 * 스켈레탈 애니메이션에서 하나의 뼈대(Bone)를 나타내는 클래스입니다.
 * 
 * 뼈대란?
 * - 캐릭터를 움직이게 하는 내부 구조입니다.
 * - 팔, 다리, 척추 같은 부분들이 각각 하나의 뼈대입니다.
 * - 각 뼈대는 시간에 따라 위치, 회전, 크기가 변합니다.
 * 
 * 키프레임(Keyframe)이란?
 * - 특정 시간에 뼈대가 어떤 상태인지 기록한 것입니다.
 * - 예: 0초에 팔이 내려가 있고, 1초에 팔이 올라가 있다면, 두 개의 키프레임이 있는 것입니다.
 * - 키프레임 사이의 값은 보간(Interpolation)으로 부드럽게 연결됩니다.
 */

#pragma once

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#include <vector>
#include <string>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/quaternion.hpp>
#include <gtx/quaternion.hpp>
#include <assimp/anim.h>
#include <assimp_glm_helpers.h>

/*
 * KeyPosition - 위치 키프레임
 * 특정 시간에 뼈대가 어느 위치에 있는지 저장합니다.
 */
struct KeyPosition
{
    glm::vec3 position;  // 위치 (x, y, z)
    float timeStamp;     // 시간 (초 단위가 아닌 틱 단위)
};

/*
 * KeyRotation - 회전 키프레임
 * 특정 시간에 뼈대가 어떻게 회전되어 있는지 저장합니다.
 * 쿼터니언(Quaternion)을 사용해서 회전을 표현합니다.
 */
struct KeyRotation
{
    glm::quat orientation;  // 회전 (쿼터니언)
    float timeStamp;        // 시간
};

/*
 * KeyScale - 크기 키프레임
 * 특정 시간에 뼈대의 크기를 저장합니다.
 */
struct KeyScale
{
    glm::vec3 scale;   // 크기 (x, y, z 방향 배율)
    float timeStamp;   // 시간
};

/*
 * Bone - 뼈대 클래스
 * 
 * 하나의 뼈대와 그 애니메이션 데이터를 관리합니다.
 */
class Bone
{
public:
    /*
     * 생성자 - Assimp에서 읽은 애니메이션 데이터로 초기화
     * 
     * @param name    뼈대 이름
     * @param id      뼈대 ID
     * @param channel 애니메이션 채널 (키프레임 데이터)
     */
    Bone(const std::string& name, int id, const aiNodeAnim* channel);

    /*
     * Update - 특정 시간의 변환 행렬 계산
     * 
     * 현재 애니메이션 시간에 맞는 위치, 회전, 크기를 보간해서
     * 최종 변환 행렬을 계산합니다.
     * 
     * @param animationTime 현재 애니메이션 시간
     */
    void Update(float animationTime);

    // 현재 변환 행렬 반환
    glm::mat4 GetLocalTransform() { return m_LocalTransform; }
    
    // 뼈대 이름 반환
    std::string GetBoneName() const { return m_Name; }
    
    // 뼈대 ID 반환
    int GetBoneID() { return m_ID; }

private:
    /*
     * GetScaleFactor - 보간 비율 계산
     * 
     * 두 키프레임 사이에서 현재 시간이 어디쯤인지 0~1 사이 값으로 반환합니다.
     * 예: 두 키프레임이 0초와 1초이고, 현재 시간이 0.5초면 0.5를 반환합니다.
     */
    float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime);
    
    // 위치 보간 (선형 보간)
    glm::mat4 InterpolatePosition(float animationTime);
    
    // 회전 보간 (구면 선형 보간 - SLERP)
    glm::mat4 InterpolateRotation(float animationTime);
    
    // 크기 보간 (선형 보간)
    glm::mat4 InterpolateScaling(float animationTime);

    // 현재 시간에 해당하는 키프레임 인덱스 찾기
    int GetPositionIndex(float animationTime);
    int GetRotationIndex(float animationTime);
    int GetScaleIndex(float animationTime);

    // 키프레임 데이터
    std::vector<KeyPosition> m_Positions;  // 위치 키프레임들
    std::vector<KeyRotation> m_Rotations;  // 회전 키프레임들
    std::vector<KeyScale> m_Scales;        // 크기 키프레임들

    int m_NumPositions;   // 위치 키프레임 개수
    int m_NumRotations;   // 회전 키프레임 개수
    int m_NumScales;      // 크기 키프레임 개수

    glm::mat4 m_LocalTransform;  // 현재 변환 행렬
    std::string m_Name;          // 뼈대 이름
    int m_ID;                    // 뼈대 ID
};
