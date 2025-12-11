/*
 * animator.h
 * 
 * 애니메이션을 재생하고 업데이트하는 클래스입니다.
 * 
 * Animator의 역할
 * - 시간에 따라 애니메이션을 진행시킵니다.
 * - 모든 뼈대의 최종 변환 행렬을 계산합니다.
 * - 계산된 행렬은 셰이더로 전달되어 정점을 변형합니다.
 * 
 * 사용 방법
 * 1. Animator 생성 시 초기 애니메이션 전달
 * 2. 매 프레임 UpdateAnimation() 호출
 * 3. GetFinalBoneMatrices()로 셰이더에 전달할 행렬 가져오기
 */

#pragma once

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#include <vector>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <animdata.h>

class Animation;  // 전방 선언

/*
 * Animator - 애니메이터 클래스
 * 
 * 애니메이션 재생과 뼈대 변환 계산을 담당합니다.
 */
class Animator
{
public:
    /*
     * 생성자
     * @param animation 재생할 애니메이션
     */
    Animator(Animation* animation);

    /*
     * UpdateAnimation - 애니메이션 업데이트
     * 
     * 매 프레임 호출해서 애니메이션을 진행시킵니다.
     * 델타 타임만큼 시간을 진행하고, 모든 뼈대의 변환 행렬을 다시 계산합니다.
     * 
     * @param dt 델타 타임 (이전 프레임과의 시간 차이, 초 단위)
     */
    void UpdateAnimation(float dt);

    /*
     * PlayAnimation - 새 애니메이션 재생
     * 
     * 현재 재생 중인 애니메이션을 바꿉니다.
     * 시간이 0으로 리셋됩니다.
     * 
     * @param pAnimation 재생할 새 애니메이션
     */
    void PlayAnimation(Animation* pAnimation);

    /*
     * CalculateBoneTransform - 뼈대 변환 계산 (재귀)
     * 
     * 노드 계층 구조를 따라가면서 각 뼈대의 최종 변환 행렬을 계산합니다.
     * 부모의 변환이 자식에게 전달됩니다 (예: 어깨가 움직이면 팔도 따라감).
     * 
     * @param node            현재 노드
     * @param parentTransform 부모의 변환 행렬
     */
    void CalculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform);

    /*
     * GetFinalBoneMatrices - 최종 뼈대 행렬 가져오기
     * 
     * 셰이더에 전달할 뼈대 변환 행렬 배열을 반환합니다.
     * 각 행렬은 정점을 해당 뼈대에 맞게 변형시킵니다.
     */
    std::vector<glm::mat4> GetFinalBoneMatrices() { return m_FinalBoneMatrices; }

    /*
     * GetCurrentAnimation - 현재 애니메이션 가져오기
     */
    Animation* GetCurrentAnimation() { return m_CurrentAnimation; }

    /*
     * Reset - 애니메이션 리셋
     * 
     * 현재 시간을 0으로 되돌립니다.
     */
    void Reset();

private:
    std::vector<glm::mat4> m_FinalBoneMatrices;  // 최종 뼈대 변환 행렬들
    Animation* m_CurrentAnimation;               // 현재 재생 중인 애니메이션
    float m_CurrentTime;                         // 현재 애니메이션 시간 (틱 단위)
    float m_DeltaTime;                           // 프레임 간 시간 차이
};
