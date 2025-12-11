/*
 * CharacterBase.h
 * 
 * 모든 캐릭터(플레이어, 적 등)가 공통으로 사용하는 기능을 정의한 클래스입니다.
 * 3D 모델 로딩, 애니메이션 관리, 화면에 그리기 등의 기능을 제공합니다.
 * 
 * Player, Enemy 같은 클래스들이 이 클래스를 상속받아서 사용합니다.
 */

#ifndef CHARACTER_BASE_H
#define CHARACTER_BASE_H

#include <string>
#include <memory>
#include <unordered_map>
#include <algorithm>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

#include "CharacterStatus.h"
#include "model.h"
#include "animation.h"
#include "animator.h"
#include "shader_m.h"

// 셰이더에서 처리할 수 있는 최대 뼈대(본) 개수
// 캐릭터 애니메이션은 뼈대를 움직여서 만드는데, GPU가 한번에 처리할 수 있는 개수에 제한이 있음
constexpr size_t MAX_BONES_SHADER = 100;

/*
 * GetIdentityBones - 기본 뼈대 행렬 반환
 * 
 * 뼈대가 없는 모델(배경 등)을 그릴 때 사용하는 기본 행렬입니다.
 * 단위 행렬(identity matrix)은 "아무 변환도 하지 않음"을 의미합니다.
 */
inline std::vector<glm::mat4>& GetIdentityBones()
{
	// static: 한 번만 생성되고 프로그램 끝날 때까지 유지됨
	static std::vector<glm::mat4> identityBones(MAX_BONES_SHADER, glm::mat4(1.0f));
	return identityBones;
}

/*
 * CharacterBase - 캐릭터 기본 클래스
 * 
 * 모든 캐릭터가 공유하는 기능들을 모아놓은 클래스입니다.
 * - 3D 모델 로딩
 * - 애니메이션 등록 및 재생
 * - 화면에 캐릭터 그리기
 * - HP 관리
 */
class CharacterBase
{
public:
	/*
	 * 생성자
	 * @param name 캐릭터 이름
	 * @param hp   시작 체력 (기본값 100)
	 */
	CharacterBase(const std::string& name, int hp = 100)
		: status_{ name, hp }
	{
	}

	// 가상 소멸자 - 상속받는 클래스에서 안전하게 삭제할 수 있게 함
	virtual ~CharacterBase() = default;

	/*
	 * LoadModel - 3D 모델 파일 로드
	 * 
	 * FBX 파일에서 3D 모델을 읽어옵니다.
	 * 
	 * @param modelPath 모델 파일 경로 (예: "assets/models/Player.fbx")
	 * @return 성공하면 true, 실패하면 false
	 */
	bool LoadModel(const std::string& modelPath)
	{
		model_ = std::make_unique<Model>(modelPath);
		return model_ != nullptr;
	}

	/*
	 * AddAnimation - 애니메이션 등록
	 * 
	 * 캐릭터가 사용할 애니메이션을 추가합니다.
	 * "idle", "attack" 같은 이름으로 구분해서 나중에 재생할 수 있습니다.
	 * 
	 * @param key  애니메이션 이름 (예: "idle", "attack")
	 * @param path 애니메이션이 들어있는 FBX 파일 경로
	 * @return 성공하면 true
	 */
	bool AddAnimation(const std::string& key, const std::string& path)
	{
		if (!model_) return false;
		
		// 애니메이션 파일 로드
		auto anim = std::make_unique<Animation>(path, model_.get());
		
		// 애니메이션 지속 시간 계산 (초 단위)
		float duration = anim->GetDuration() / std::max(anim->GetTicksPerSecond(), 1.0f);
		if (duration <= 0.0f) duration = 1.0f;
		
		durations_[key] = duration;
		animations_[key] = std::move(anim);

		// 첫 번째 애니메이션 등록 시 Animator 생성
		if (!animator_)
			animator_ = std::make_unique<Animator>(animations_[key].get());

		return true;
	}

	/*
	 * PlayAnimation - 애니메이션 재생
	 * 
	 * 등록된 애니메이션을 재생합니다.
	 * 
	 * @param key 재생할 애니메이션 이름
	 * @return 성공하면 true
	 */
	bool PlayAnimation(const std::string& key)
	{
		auto it = animations_.find(key);
		if (it == animations_.end() || !animator_) return false;
		
		animator_->PlayAnimation(it->second.get());
		currentKey_ = key;
		return true;
	}

	/*
	 * Update - 애니메이션 업데이트
	 * 
	 * 매 프레임마다 호출해서 애니메이션을 진행시킵니다.
	 * 
	 * @param dt 델타 타임 (이전 프레임과의 시간 차이, 초 단위)
	 */
	void Update(float dt)
	{
		if (animator_) animator_->UpdateAnimation(dt);
	}

	/*
	 * GetFinalBoneMatrices - 최종 뼈대 변환 행렬 가져오기
	 * 
	 * 현재 애니메이션 프레임에서 각 뼈대가 어떻게 변환되었는지를 담은 행렬들입니다.
	 * 셰이더에서 정점을 변환할 때 사용합니다.
	 */
	const std::vector<glm::mat4>& GetFinalBoneMatrices() const
	{
		static std::vector<glm::mat4> empty;
		if (animator_) return animator_->GetFinalBoneMatrices();
		return empty;
	}

	// 모델 포인터 반환
	Model* GetModel() const { return model_.get(); }
	
	// 애니메이터 포인터 반환
	Animator* GetAnimator() const { return animator_.get(); }

	/*
	 * GetDuration - 애니메이션 지속 시간 가져오기
	 * 
	 * @param key 애니메이션 이름
	 * @return 지속 시간 (초 단위)
	 */
	float GetDuration(const std::string& key) const
	{
		auto it = durations_.find(key);
		if (it == durations_.end()) return 1.0f;
		return it->second;
	}

	// 캐릭터 상태 (이름, HP) 반환
	CharacterStatus& Status() { return status_; }
	const CharacterStatus& Status() const { return status_; }

	/*
	 * RenderDepth - 그림자용 깊이 렌더링
	 * 
	 * 그림자를 만들기 위해 빛의 시점에서 캐릭터를 그립니다.
	 * 색상은 무시하고 깊이(거리) 정보만 기록합니다.
	 * 
	 * @param shader   깊이 렌더링용 셰이더
	 * @param position 캐릭터 위치
	 * @param scale    크기 배율
	 */
	void RenderDepth(Shader& shader, const glm::vec3& position, float scale)
	{
		if (!animator_ || !model_) return;

		// 모델 변환 행렬 생성 (위치 이동 + 크기 조절)
		glm::mat4 modelMat = glm::mat4(1.0f);
		modelMat = glm::translate(modelMat, position);
		modelMat = glm::scale(modelMat, glm::vec3(scale));
		shader.setMat4("model", modelMat);

		// 뼈대 변환 행렬을 셰이더에 전달
		auto transforms = animator_->GetFinalBoneMatrices();
		size_t boneCount = std::min(transforms.size(), static_cast<size_t>(MAX_BONES_SHADER));
		for (size_t i = 0; i < boneCount; ++i)
			shader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);

		model_->Draw(shader);
	}

	/*
	 * Render - 화면에 캐릭터 그리기
	 * 
	 * 조명이 적용된 캐릭터를 화면에 그립니다.
	 * 
	 * @param shader    렌더링용 셰이더
	 * @param position  캐릭터 위치
	 * @param scale     크기 배율
	 * @param rotationY Y축 회전 각도 (도 단위)
	 */
	void Render(Shader& shader, const glm::vec3& position, float scale, float rotationY = 0.0f)
	{
		if (!animator_ || !model_) return;

		// 모델 변환 행렬 생성 (위치 이동 + 회전 + 크기 조절)
		glm::mat4 modelMat = glm::mat4(1.0f);
		modelMat = glm::translate(modelMat, position);
		if (rotationY != 0.0f)
			modelMat = glm::rotate(modelMat, glm::radians(rotationY), glm::vec3(0, 1, 0));
		modelMat = glm::scale(modelMat, glm::vec3(scale));
		shader.setMat4("model", modelMat);

		// 뼈대 변환 행렬을 셰이더에 전달
		auto transforms = animator_->GetFinalBoneMatrices();
		size_t boneCount = std::min(transforms.size(), static_cast<size_t>(MAX_BONES_SHADER));
		for (size_t i = 0; i < boneCount; ++i)
			shader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);

		model_->Draw(shader);
	}

protected:
	CharacterStatus status_;                                              // 캐릭터 상태 (이름, HP)
	std::unique_ptr<Model> model_;                                        // 3D 모델
	std::unique_ptr<Animator> animator_;                                  // 애니메이션 재생기
	std::unordered_map<std::string, std::unique_ptr<Animation>> animations_; // 애니메이션 목록
	std::unordered_map<std::string, float> durations_;                    // 애니메이션별 지속 시간
	std::string currentKey_ = "idle";                                     // 현재 재생 중인 애니메이션
};

#endif // CHARACTER_BASE_H
