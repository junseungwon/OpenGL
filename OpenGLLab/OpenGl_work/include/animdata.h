#pragma once

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <string>
#include <vector>

const int MAX_BONES = 100;

struct BoneInfo
{
    int id;
    glm::mat4 offset;
};

struct AssimpNodeData
{
    glm::mat4 transformation;
    std::string name;
    int childrenCount;
    std::vector<AssimpNodeData> children;
};

