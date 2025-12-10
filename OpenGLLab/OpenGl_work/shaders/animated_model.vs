#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 5) in ivec4 aBoneIDs;
layout (location = 6) in vec4 aWeights;

const int MAX_BONES = 100;

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;
uniform mat4 finalBonesMatrices[MAX_BONES];

mat4 getBoneTransform()
{
    float wSum = aWeights[0] + aWeights[1] + aWeights[2] + aWeights[3];
    if (wSum <= 0.0001)
        return mat4(1.0); // no bone influences -> identity

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

    vec4 localPos = boneTransform * vec4(aPos, 1.0);
    vec3 localNormal = mat3(boneTransform) * aNormal;

    vec4 worldPos = model * localPos;

    vs_out.FragPos    = worldPos.xyz;
    vs_out.Normal     = mat3(transpose(inverse(model))) * localNormal;
    vs_out.TexCoords  = aTexCoords;
    vs_out.FragPosLightSpace = lightSpaceMatrix * worldPos;

    gl_Position = projection * view * worldPos;
}

