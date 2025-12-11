#include <bone.h>

// Bone 생성자
// FBX 파일에서 읽은 뼈대 애니메이션 데이터로 Bone 객체를 초기화합니다.
// 위치, 회전, 크기 키프레임을 모두 읽어서 저장합니다.
// @param name 뼈대 이름
// @param id 뼈대 고유 ID
// @param channel Assimp의 애니메이션 채널 (키프레임 데이터 포함)
Bone::Bone(const std::string& name, int id, const aiNodeAnim* channel)
	: m_Name(name), m_ID(id), m_LocalTransform(1.0f)
{
	// ========== 위치 키프레임 읽기 ==========
	// 특정 시간에 뼈대가 어디에 있는지 저장
	m_NumPositions = channel->mNumPositionKeys;
	for (int positionIndex = 0; positionIndex < m_NumPositions; ++positionIndex)
	{
		aiVector3D aiPosition = channel->mPositionKeys[positionIndex].mValue;  // 위치 (x, y, z)
		float timeStamp = (float)channel->mPositionKeys[positionIndex].mTime;   // 시간 (틱)
		
		KeyPosition data;
		data.position = AssimpGLMHelpers::GetGLMVec(aiPosition);  // Assimp → GLM 변환
		data.timeStamp = timeStamp;
		m_Positions.push_back(data);
	}

	// ========== 회전 키프레임 읽기 ==========
	// 특정 시간에 뼈대가 어떻게 회전되어 있는지 저장 (쿼터니언 사용)
	m_NumRotations = channel->mNumRotationKeys;
	for (int rotationIndex = 0; rotationIndex < m_NumRotations; ++rotationIndex)
	{
		aiQuaternion aiOrientation = channel->mRotationKeys[rotationIndex].mValue;  // 회전 (쿼터니언)
		float timeStamp = (float)channel->mRotationKeys[rotationIndex].mTime;        // 시간 (틱)
		
		KeyRotation data;
		data.orientation = AssimpGLMHelpers::GetGLMQuat(aiOrientation);  // Assimp → GLM 변환
		data.timeStamp = timeStamp;
		m_Rotations.push_back(data);
	}

	// ========== 크기 키프레임 읽기 ==========
	// 특정 시간에 뼈대의 크기가 얼마인지 저장
	m_NumScales = channel->mNumScalingKeys;
	for (int keyIndex = 0; keyIndex < m_NumScales; ++keyIndex)
	{
		aiVector3D scale = channel->mScalingKeys[keyIndex].mValue;  // 크기 (x, y, z 배율)
		float timeStamp = (float)channel->mScalingKeys[keyIndex].mTime;  // 시간 (틱)
		
		KeyScale data;
		data.scale = AssimpGLMHelpers::GetGLMVec(scale);  // Assimp → GLM 변환
		data.timeStamp = timeStamp;
		m_Scales.push_back(data);
	}
}

// 현재 시간에 맞는 변환 행렬 계산
// 키프레임 사이에서 보간하여 최종 변환 행렬을 계산합니다.
// @param animationTime 현재 애니메이션 시간 (틱 단위)
void Bone::Update(float animationTime)
{
	// 위치, 회전, 크기를 각각 보간
	glm::mat4 translation = InterpolatePosition(animationTime);
	glm::mat4 rotation = InterpolateRotation(animationTime);
	glm::mat4 scale = InterpolateScaling(animationTime);
	
	// 최종 변환 행렬 = 이동 × 회전 × 크기
	m_LocalTransform = translation * rotation * scale;
}

// 현재 시간에 해당하는 위치 키프레임 인덱스 찾기
// 두 키프레임 사이에서 보간하기 위해 현재 시간이 어느 키프레임 사이에 있는지 찾습니다.
// @param animationTime 현재 애니메이션 시간
// @return 시작 키프레임 인덱스
int Bone::GetPositionIndex(float animationTime)
{
	// 키프레임 배열을 순회하며 현재 시간이 어느 구간에 있는지 찾기
	for (int index = 0; index < m_NumPositions - 1; ++index)
	{
		// 다음 키프레임의 시간보다 작으면 이 구간에 있음
		if (animationTime < m_Positions[index + 1].timeStamp)
			return index;
	}
	// 마지막 키프레임 반환
	return m_NumPositions - 1;
}

// 현재 시간에 해당하는 회전 키프레임 인덱스 찾기
// @param animationTime 현재 애니메이션 시간
// @return 시작 키프레임 인덱스
int Bone::GetRotationIndex(float animationTime)
{
	for (int index = 0; index < m_NumRotations - 1; ++index)
	{
		if (animationTime < m_Rotations[index + 1].timeStamp)
			return index;
	}
	return m_NumRotations - 1;
}

// 현재 시간에 해당하는 크기 키프레임 인덱스 찾기
// @param animationTime 현재 애니메이션 시간
// @return 시작 키프레임 인덱스
int Bone::GetScaleIndex(float animationTime)
{
	for (int index = 0; index < m_NumScales - 1; ++index)
	{
		if (animationTime < m_Scales[index + 1].timeStamp)
			return index;
	}
	return m_NumScales - 1;
}

// 보간 비율 계산
// 두 키프레임 사이에서 현재 시간이 어디쯤인지 0.0 ~ 1.0 사이 값으로 반환합니다.
// 예: 키프레임 0(0틱)과 키프레임 1(10틱) 사이에서 현재 시간이 5틱이면 0.5 반환
// @param lastTimeStamp 시작 키프레임 시간
// @param nextTimeStamp 끝 키프레임 시간
// @param animationTime 현재 시간
// @return 보간 비율 (0.0 ~ 1.0)
float Bone::GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime)
{
	float scaleFactor = 0.0f;
	float midWayLength = animationTime - lastTimeStamp;  // 시작부터 현재까지의 거리
	float framesDiff = nextTimeStamp - lastTimeStamp;    // 두 키프레임 사이의 거리
	scaleFactor = midWayLength / framesDiff;             // 비율 계산
	return scaleFactor;
}

// 위치 보간 (선형 보간 - Lerp)
// 두 위치 키프레임 사이에서 현재 시간에 맞는 위치를 계산합니다.
// @param animationTime 현재 애니메이션 시간
// @return 위치 변환 행렬
glm::mat4 Bone::InterpolatePosition(float animationTime)
{
	// 키프레임이 하나만 있으면 그대로 사용
	if (m_NumPositions == 1)
		return glm::translate(glm::mat4(1.0f), m_Positions[0].position);

	// 현재 시간이 어느 키프레임 사이에 있는지 찾기
	int p0Index = GetPositionIndex(animationTime);  // 시작 키프레임
	int p1Index = p0Index + 1;                      // 끝 키프레임
	
	// 보간 비율 계산 (0.0 ~ 1.0)
	float scaleFactor = GetScaleFactor(m_Positions[p0Index].timeStamp, m_Positions[p1Index].timeStamp, animationTime);
	
	// 선형 보간 (Lerp)
	glm::vec3 finalPosition = glm::mix(m_Positions[p0Index].position, m_Positions[p1Index].position, scaleFactor);
	
	// 위치 변환 행렬 반환
	return glm::translate(glm::mat4(1.0f), finalPosition);
}

// 회전 보간 (구면 선형 보간 - SLERP)
// 두 회전 키프레임 사이에서 현재 시간에 맞는 회전을 계산합니다.
// 회전은 쿼터니언으로 표현되며, 구면에서 부드럽게 보간합니다.
// @param animationTime 현재 애니메이션 시간
// @return 회전 변환 행렬
glm::mat4 Bone::InterpolateRotation(float animationTime)
{
	// 키프레임이 하나만 있으면 그대로 사용
	if (m_NumRotations == 1)
	{
		auto rotation = glm::normalize(m_Rotations[0].orientation);
		return glm::toMat4(rotation);
	}

	// 현재 시간이 어느 키프레임 사이에 있는지 찾기
	int p0Index = GetRotationIndex(animationTime);
	int p1Index = p0Index + 1;
	
	// 보간 비율 계산
	float scaleFactor = GetScaleFactor(m_Rotations[p0Index].timeStamp, m_Rotations[p1Index].timeStamp, animationTime);
	
	// 구면 선형 보간 (SLERP) - 회전은 쿼터니언으로
	glm::quat finalRotation = glm::slerp(m_Rotations[p0Index].orientation, m_Rotations[p1Index].orientation, scaleFactor);
	finalRotation = glm::normalize(finalRotation);  // 정규화 (길이를 1로)
	
	// 쿼터니언 → 행렬 변환
	return glm::toMat4(finalRotation);
}

// 크기 보간 (선형 보간 - Lerp)
// 두 크기 키프레임 사이에서 현재 시간에 맞는 크기를 계산합니다.
// @param animationTime 현재 애니메이션 시간
// @return 크기 변환 행렬
glm::mat4 Bone::InterpolateScaling(float animationTime)
{
	// 키프레임이 하나만 있으면 그대로 사용
	if (m_NumScales == 1)
		return glm::scale(glm::mat4(1.0f), m_Scales[0].scale);

	// 현재 시간이 어느 키프레임 사이에 있는지 찾기
	int p0Index = GetScaleIndex(animationTime);
	int p1Index = p0Index + 1;
	
	// 보간 비율 계산
	float scaleFactor = GetScaleFactor(m_Scales[p0Index].timeStamp, m_Scales[p1Index].timeStamp, animationTime);
	
	// 선형 보간 (Lerp)
	glm::vec3 finalScale = glm::mix(m_Scales[p0Index].scale, m_Scales[p1Index].scale, scaleFactor);
	
	// 크기 변환 행렬 반환
	return glm::scale(glm::mat4(1.0f), finalScale);
}

