/*
 * Background.h
 * 
 * 배경(환경) 모델을 관리하는 클래스입니다.
 * 지형, 건물 등 움직이지 않는 배경 오브젝트를 표시합니다.
 * 
 * 캐릭터와 달리 애니메이션이 없어서 단순하게 구현되어 있습니다.
 */

#ifndef BACKGROUND_H
#define BACKGROUND_H

#include <memory>
#include <string>
#include <algorithm>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

#include "model.h"
#include "shader_m.h"
#include "CharacterBase.h"  // GetIdentityBones(), MAX_BONES_SHADER 사용

class Background
{
public:
	Background() = default;

	/*
	 * Init - 배경 모델 로드
	 * 
	 * FBX 파일에서 배경 모델을 읽어옵니다.
	 * 
	 * @param modelPath 모델 파일 경로
	 * @return 성공하면 true
	 */
	bool Init(const std::string& modelPath)
	{
		model_ = std::make_unique<Model>(modelPath);
		return model_ != nullptr;
	}

	/*
	 * SetTransform - 위치/회전/크기 설정
	 * 
	 * 배경의 위치, 회전, 크기를 설정합니다.
	 * 
	 * @param position 위치 (x, y, z)
	 * @param rotation 회전 각도 (x, y, z축 기준, 도 단위)
	 * @param scale    크기 배율
	 */
	void SetTransform(const glm::vec3& position, const glm::vec3& rotation, float scale)
	{
		position_ = position;
		rotation_ = rotation;
		scale_ = scale;
	}

	/*
	 * RenderDepth - 그림자용 깊이 렌더링
	 * 
	 * 그림자 맵을 만들기 위해 빛의 시점에서 배경을 그립니다.
	 * 
	 * @param shader        깊이 렌더링용 셰이더
	 * @param scaleOverride 기본 스케일 대신 사용할 스케일 (0이면 기본값 사용)
	 */
	void RenderDepth(Shader& shader, float scaleOverride = 0.0f)
	{
		if (!model_) return;

		// 스케일 결정: scaleOverride가 있으면 그걸 사용
		float useScale = (scaleOverride > 0.0f) ? scaleOverride : scale_;

		// 모델 변환 행렬 생성
		glm::mat4 modelMat = glm::mat4(1.0f);
		modelMat = glm::translate(modelMat, position_);
		modelMat = glm::rotate(modelMat, glm::radians(rotation_.x), glm::vec3(1, 0, 0));
		modelMat = glm::rotate(modelMat, glm::radians(rotation_.y), glm::vec3(0, 1, 0));
		modelMat = glm::rotate(modelMat, glm::radians(rotation_.z), glm::vec3(0, 0, 1));
		modelMat = glm::scale(modelMat, glm::vec3(useScale));
		shader.setMat4("model", modelMat);

		// 배경은 뼈대가 없으므로 단위 행렬 사용
		auto& identityBones = GetIdentityBones();
		size_t boneCount = std::min(identityBones.size(), static_cast<size_t>(MAX_BONES_SHADER));
		for (size_t i = 0; i < boneCount; ++i)
			shader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", identityBones[i]);

		model_->Draw(shader);
	}

	/*
	 * Render - 화면에 배경 그리기
	 * 
	 * 조명이 적용된 배경을 화면에 그립니다.
	 * 
	 * @param shader 렌더링용 셰이더
	 */
	void Render(Shader& shader)
	{
		if (!model_) return;

		// 모델 변환 행렬 생성
		glm::mat4 modelMat = glm::mat4(1.0f);
		modelMat = glm::translate(modelMat, position_);
		modelMat = glm::rotate(modelMat, glm::radians(rotation_.x), glm::vec3(1, 0, 0));
		modelMat = glm::rotate(modelMat, glm::radians(rotation_.y), glm::vec3(0, 1, 0));
		modelMat = glm::rotate(modelMat, glm::radians(rotation_.z), glm::vec3(0, 0, 1));
		modelMat = glm::scale(modelMat, glm::vec3(scale_));
		shader.setMat4("model", modelMat);

		// 배경은 뼈대가 없으므로 단위 행렬 사용
		auto& identityBones = GetIdentityBones();
		size_t boneCount = std::min(identityBones.size(), static_cast<size_t>(MAX_BONES_SHADER));
		for (size_t i = 0; i < boneCount; ++i)
			shader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", identityBones[i]);

		model_->Draw(shader);
	}

	// 모델 포인터 반환
	Model* GetModel() const { return model_.get(); }

private:
	std::unique_ptr<Model> model_;              // 3D 모델
	glm::vec3 position_ = glm::vec3(0.0f);      // 위치
	glm::vec3 rotation_ = glm::vec3(0.0f);      // 회전 (도 단위)
	float scale_ = 1.0f;                        // 크기 배율
};

#endif // BACKGROUND_H
