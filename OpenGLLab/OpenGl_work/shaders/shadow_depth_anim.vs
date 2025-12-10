#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 5) in ivec4 aBoneIDs;
layout (location = 6) in vec4 aWeights;

uniform mat4 model;
uniform mat4 lightSpaceMatrix;
const int MAX_BONES = 100;
uniform mat4 finalBonesMatrices[MAX_BONES];

mat4 getBoneTransform()
{
    float wSum = aWeights[0] + aWeights[1] + aWeights[2] + aWeights[3];
    if (wSum <= 0.0001)
        return mat4(1.0);

    mat4 boneTransform = mat4(0.0);
    boneTransform += finalBonesMatrices[aBoneIDs[0]] * aWeights[0];
    boneTransform += finalBonesMatrices[aBoneIDs[1]] * aWeights[1];
    boneTransform += finalBonesMatrices[aBoneIDs[2]] * aWeights[2];
    boneTransform += finalBonesMatrices[aBoneIDs[3]] * aWeights[3];
    return boneTransform;
}

void main()
{
    mat4 boneTransform = getBoneTransform();
    vec4 worldPos = model * boneTransform * vec4(aPos, 1.0);
    gl_Position = lightSpaceMatrix * worldPos;
}

