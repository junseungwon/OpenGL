#include <animation.h>
#include <iostream>

// Animation 생성자
// FBX 파일에서 애니메이션 데이터를 로드하고 초기화합니다.
// @param animationPath 애니메이션 파일 경로 (FBX)
// @param model 모델 객체 (뼈대 정보 공유)
Animation::Animation(const std::string& animationPath, Model* model)
{
	// Assimp으로 FBX 파일 읽기
	Assimp::Importer importer;
	// 삼각형화, UV 뒤집기, 뼈대 가중치 제한 옵션 사용
	const aiScene* scene = importer.ReadFile(animationPath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_LimitBoneWeights);

	// 파일 로드 실패 체크
	if (!scene || !scene->mRootNode || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)
	{
		std::cout << "ERROR::ANIMATION::ASSIMP:: " << importer.GetErrorString() << std::endl;
		return;
	}

	// 첫 번째 애니메이션 클립 가져오기 (FBX 파일에는 보통 하나의 애니메이션이 있음)
	const aiAnimation* animation = scene->mAnimations[0];
	
	// 애니메이션 길이 (틱 단위)
	m_Duration = (float)animation->mDuration;
	// 초당 틱 수 (애니메이션 속도, 없으면 기본값 25 사용)
	m_TicksPerSecond = animation->mTicksPerSecond != 0 ? (int)animation->mTicksPerSecond : 25;

	// 전역 역행렬 계산 (나중에 사용할 수 있지만 현재는 사용하지 않음)
	aiMatrix4x4 globalInverse = scene->mRootNode->mTransformation;
	globalInverse.Inverse();
	
	// 뼈대 계층 구조 읽기 (부모-자식 관계)
	ReadHierarchyData(m_RootNode, scene->mRootNode);
	
	// 각 뼈대의 키프레임 데이터 읽기
	ReadMissingBones(animation, *model);
}

// 이름으로 뼈대 찾기
// @param name 찾을 뼈대 이름
// @return 뼈대 포인터 (없으면 nullptr)
Bone* Animation::FindBone(const std::string& name)
{
	// 뼈대 목록에서 이름으로 검색
	auto iter = std::find_if(m_Bones.begin(), m_Bones.end(),
		[&](const Bone& bone) { return bone.GetBoneName() == name; });
	if (iter == m_Bones.end()) return nullptr;
	return &(*iter);
}

// 애니메이션 파일에서 뼈대 정보 읽기
// 모델 파일에는 없지만 애니메이션 파일에 있는 뼈대를 추가합니다.
// 각 뼈대의 키프레임 데이터를 Bone 객체로 생성합니다.
// @param animation Assimp 애니메이션 객체
// @param model 모델 객체 (뼈대 정보 맵 공유)
void Animation::ReadMissingBones(const aiAnimation* animation, Model& model)
{
	// 애니메이션 채널 개수 (각 채널 = 하나의 뼈대)
	int size = animation->mNumChannels;

	// 모델의 뼈대 정보 맵 가져오기 (공유)
	auto& boneInfoMap = model.GetBoneInfoMap();
	int& boneCount = model.GetBoneCount();

	// 각 채널(뼈대) 처리
	for (int i = 0; i < size; i++)
	{
		auto channel = animation->mChannels[i];  // aiNodeAnim (뼈대 애니메이션 데이터)
		std::string boneName = channel->mNodeName.data;

		// 모델에 없는 뼈대면 추가
		if (boneInfoMap.find(boneName) == boneInfoMap.end())
		{
			boneInfoMap[boneName].id = boneCount;           // 뼈대 ID 할당
			boneInfoMap[boneName].offset = glm::mat4(1.0f);  // 기본 오프셋 행렬
			boneCount++;
		}
		
		// Bone 객체 생성 (키프레임 데이터 포함)
		m_Bones.push_back(Bone(boneName, boneInfoMap[boneName].id, channel));
	}
	
	// 뼈대 정보 맵 복사
	m_BoneInfoMap = boneInfoMap;
}

// 뼈대 계층 구조 읽기 (재귀 함수)
// Assimp의 노드 트리를 우리가 사용하는 형식으로 변환합니다.
// 부모-자식 관계를 유지하면서 전체 구조를 복사합니다.
// @param dest 목적지 (우리 형식의 노드 데이터)
// @param src 소스 (Assimp 노드)
void Animation::ReadHierarchyData(AssimpNodeData& dest, const aiNode* src)
{
	// 노드 이름 복사
	dest.name = src->mName.data;
	// 변환 행렬 변환 (Assimp → GLM)
	dest.transformation = AssimpGLMHelpers::ConvertMatrixToGLMFormat(src->mTransformation);
	// 자식 개수 저장
	dest.childrenCount = src->mNumChildren;

	// 모든 자식 노드를 재귀적으로 처리
	for (unsigned int i = 0; i < src->mNumChildren; ++i)
	{
		AssimpNodeData newData;
		ReadHierarchyData(newData, src->mChildren[i]);
		dest.children.push_back(newData);
	}
}

