#include <animation.h>
#include <iostream>

Animation::Animation(const std::string& animationPath, Model* model)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(animationPath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_LimitBoneWeights);

	if (!scene || !scene->mRootNode || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)
	{
		std::cout << "ERROR::ANIMATION::ASSIMP:: " << importer.GetErrorString() << std::endl;
		return;
	}

	const aiAnimation* animation = scene->mAnimations[0];
	m_Duration = (float)animation->mDuration;
	m_TicksPerSecond = animation->mTicksPerSecond != 0 ? (int)animation->mTicksPerSecond : 25;

	aiMatrix4x4 globalInverse = scene->mRootNode->mTransformation;
	globalInverse.Inverse();
	ReadHierarchyData(m_RootNode, scene->mRootNode);
	ReadMissingBones(animation, *model);
}

Bone* Animation::FindBone(const std::string& name)
{
	auto iter = std::find_if(m_Bones.begin(), m_Bones.end(),
		[&](const Bone& bone) { return bone.GetBoneName() == name; });
	if (iter == m_Bones.end()) return nullptr;
	return &(*iter);
}

void Animation::ReadMissingBones(const aiAnimation* animation, Model& model)
{
	int size = animation->mNumChannels;

	auto& boneInfoMap = model.GetBoneInfoMap();
	int& boneCount = model.GetBoneCount();

	for (int i = 0; i < size; i++)
	{
		auto channel = animation->mChannels[i];
		std::string boneName = channel->mNodeName.data;

		if (boneInfoMap.find(boneName) == boneInfoMap.end())
		{
			boneInfoMap[boneName].id = boneCount;
			boneInfoMap[boneName].offset = glm::mat4(1.0f);
			boneCount++;
		}
		m_Bones.push_back(Bone(boneName, boneInfoMap[boneName].id, channel));
	}
	m_BoneInfoMap = boneInfoMap;
}

void Animation::ReadHierarchyData(AssimpNodeData& dest, const aiNode* src)
{
	dest.name = src->mName.data;
	dest.transformation = AssimpGLMHelpers::ConvertMatrixToGLMFormat(src->mTransformation);
	dest.childrenCount = src->mNumChildren;

	for (unsigned int i = 0; i < src->mNumChildren; ++i)
	{
		AssimpNodeData newData;
		ReadHierarchyData(newData, src->mChildren[i]);
		dest.children.push_back(newData);
	}
}

