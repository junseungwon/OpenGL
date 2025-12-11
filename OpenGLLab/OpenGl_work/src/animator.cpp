#include <animator.h>
#include <animation.h>
#include <bone.h>
#include <cmath>

// Animator 생성자
// 애니메이션 재생기를 초기화하고 최종 변환 행렬 배열을 준비합니다.
// @param animation 재생할 애니메이션
Animator::Animator(Animation* animation)
	: m_CurrentAnimation(animation), m_CurrentTime(0.0f), m_DeltaTime(0.0f)
{
	// 최종 변환 행렬 배열 초기화 (최대 100개 뼈대)
	m_FinalBoneMatrices.reserve(MAX_BONES);
	for (int i = 0; i < MAX_BONES; i++)
		m_FinalBoneMatrices.push_back(glm::mat4(1.0f));  // 단위 행렬로 초기화
}

// 애니메이션 업데이트
// 매 프레임 호출하여 애니메이션 시간을 진행시키고 모든 뼈대의 변환 행렬을 계산합니다.
// @param dt 델타 타임 (이전 프레임과의 시간 차이, 초 단위)
void Animator::UpdateAnimation(float dt)
{
	m_DeltaTime = dt;
	if (m_CurrentAnimation)
	{
		// 애니메이션 시간 업데이트
		// 초당 틱 수를 곱해서 실제 시간을 틱 단위로 변환
		m_CurrentTime += m_CurrentAnimation->GetTicksPerSecond() * dt;
		
		// 애니메이션이 끝나면 처음부터 다시 (루프)
		m_CurrentTime = fmod(m_CurrentTime, m_CurrentAnimation->GetDuration());
		
		// 뼈대 변환 계산 시작 (루트 노드부터, 부모 변환은 단위 행렬)
		CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f));
	}
}

// 새 애니메이션 재생
// 현재 재생 중인 애니메이션을 바꾸고 시간을 0으로 리셋합니다.
// @param pAnimation 재생할 새 애니메이션
void Animator::PlayAnimation(Animation* pAnimation)
{
	m_CurrentAnimation = pAnimation;
	m_CurrentTime = 0.0f;  // 시간 리셋
}

// 뼈대 변환 계산 (재귀 함수)
// 노드 계층 구조를 따라가면서 각 뼈대의 최종 변환 행렬을 계산합니다.
// 부모의 변환이 자식에게 전달됩니다 (예: 어깨가 움직이면 팔도 따라감).
// @param node 현재 노드
// @param parentTransform 부모의 변환 행렬
void Animator::CalculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform)
{
	std::string nodeName = node->name;
	glm::mat4 nodeTransform = node->transformation;  // 기본 변환 (애니메이션 없을 때)

	// 이 노드가 애니메이션되는 뼈대인가?
	Bone* bone = m_CurrentAnimation->FindBone(nodeName);
	if (bone)
	{
		// 현재 시간에 맞는 변환 계산 (키프레임 보간)
		bone->Update(m_CurrentTime);
		nodeTransform = bone->GetLocalTransform();  // 애니메이션된 변환 사용
	}

	// 글로벌 변환 계산
	// 부모 변환 × 로컬 변환 = 이 뼈대의 글로벌 변환
	// 예: 어깨(부모)가 30도 회전, 팔(자식)이 45도 회전 → 팔의 최종 회전 = 75도
	glm::mat4 globalTransformation = parentTransform * nodeTransform;

	// 이 뼈대가 정점에 영향을 주는 뼈대인가?
	auto boneInfoMap = m_CurrentAnimation->GetBoneIDMap();
	if (boneInfoMap.find(nodeName) != boneInfoMap.end())
	{
		int index = boneInfoMap[nodeName].id;  // 뼈대 ID
		
		// 배열 범위 체크
		if (index >= 0 && static_cast<size_t>(index) < m_FinalBoneMatrices.size())
		{
			// 최종 변환 행렬 계산
			// 오프셋 행렬: 정점을 뼈대의 로컬 공간으로 변환
			// 최종 행렬: 뼈대 변환을 정점에 적용할 수 있게 만든 행렬
			glm::mat4 offset = boneInfoMap[nodeName].offset;
			m_FinalBoneMatrices[index] = globalTransformation * offset;
		}
		// else: 뼈대 인덱스가 MAX_BONES를 초과하면 안전하게 무시
	}

	// 자식 뼈대들도 재귀적으로 처리
	// 자식에게는 이 뼈대의 글로벌 변환을 부모 변환으로 전달
	for (int i = 0; i < node->childrenCount; i++)
		CalculateBoneTransform(&node->children[i], globalTransformation);
}

// 애니메이션 리셋
// 현재 시간을 0으로 되돌리고 모든 뼈대 변환 행렬을 단위 행렬로 초기화합니다.
void Animator::Reset()
{
	m_CurrentTime = 0.0f;
	// 모든 변환 행렬을 단위 행렬로 초기화
	for (auto& m : m_FinalBoneMatrices) m = glm::mat4(1.0f);
}

