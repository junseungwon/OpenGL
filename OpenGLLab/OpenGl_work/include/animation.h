/*
 * animation.h
 * 
 * 하나의 애니메이션 클립을 나타내는 클래스입니다.
 * 
 * 애니메이션 클립이란?
 * - "걷기", "뛰기", "공격" 같은 하나의 동작입니다.
 * - FBX 파일에서 애니메이션 데이터를 읽어옵니다.
 * - 여러 뼈대의 키프레임 데이터를 가지고 있습니다.
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <algorithm>

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include <animdata.h>
#include <bone.h>
#include <model.h>

/*
 * Animation - 애니메이션 클래스
 * 
 * FBX 파일에서 애니메이션 데이터를 로드하고 관리합니다.
 * 모든 뼈대의 키프레임 데이터와 계층 구조를 저장합니다.
 */
class Animation
{
public:
    Animation() = default;

    /*
     * 생성자 - 애니메이션 파일 로드
     * 
     * @param animationPath 애니메이션 FBX 파일 경로
     * @param model         이 애니메이션을 사용할 모델 (뼈대 정보 공유)
     */
    Animation(const std::string& animationPath, Model* model);

    /*
     * FindBone - 이름으로 뼈대 찾기
     * 
     * @param name 찾을 뼈대 이름
     * @return 뼈대 포인터 (없으면 nullptr)
     */
    Bone* FindBone(const std::string& name);

    // 초당 틱 수 (애니메이션 속도 계산에 사용)
    inline float GetTicksPerSecond() { return static_cast<float>(m_TicksPerSecond); }
    
    // 애니메이션 전체 길이 (틱 단위)
    inline float GetDuration() { return m_Duration; }
    
    // 노드 계층 구조의 루트
    inline const AssimpNodeData& GetRootNode() { return m_RootNode; }
    
    // 뼈대 ID 맵 (이름 → 뼈대 정보)
    inline const std::map<std::string, BoneInfo>& GetBoneIDMap() { return m_BoneInfoMap; }

private:
    /*
     * ReadMissingBones - 모델에 없는 뼈대 정보 읽기
     * 
     * 애니메이션 파일에는 있지만 모델에는 없는 뼈대를 추가합니다.
     */
    void ReadMissingBones(const aiAnimation* animation, Model& model);

    /*
     * ReadHierarchyData - 노드 계층 구조 읽기
     * 
     * Assimp 노드 구조를 우리가 사용하는 형식으로 변환합니다.
     */
    void ReadHierarchyData(AssimpNodeData& dest, const aiNode* src);

    float m_Duration;      // 애니메이션 길이 (틱 단위)
    int m_TicksPerSecond;  // 초당 틱 수
    
    std::vector<Bone> m_Bones;                      // 뼈대 목록
    AssimpNodeData m_RootNode;                      // 루트 노드
    std::map<std::string, BoneInfo> m_BoneInfoMap;  // 뼈대 이름 → 정보 맵
};
