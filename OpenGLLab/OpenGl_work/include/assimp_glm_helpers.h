/*
 * assimp_glm_helpers.h
 * 
 * Assimp 라이브러리의 데이터 타입을 GLM 타입으로 변환하는 헬퍼 함수들입니다.
 * 
 * Assimp: 3D 모델 파일(FBX, OBJ 등)을 읽어오는 라이브러리
 * GLM: OpenGL에서 사용하는 수학 라이브러리 (벡터, 행렬 등)
 * 
 * Assimp은 자체 데이터 타입을 사용하므로, OpenGL에서 쓰려면 변환이 필요합니다.
 */

#pragma once

#include <assimp/quaternion.h>
#include <assimp/matrix4x4.h>
#include <glm.hpp>
#include <gtc/quaternion.hpp>

namespace AssimpGLMHelpers
{
    /*
     * ConvertMatrixToGLMFormat - Assimp 행렬을 GLM 행렬로 변환
     * 
     * 4x4 변환 행렬을 변환합니다.
     * Assimp은 행 우선(row-major), GLM은 열 우선(column-major)이라서
     * 데이터를 재배치해야 합니다.
     * 
     * @param from Assimp 4x4 행렬
     * @return GLM 4x4 행렬
     */
    inline glm::mat4 ConvertMatrixToGLMFormat(const aiMatrix4x4& from)
    {
        glm::mat4 to;
        // Assimp은 행 우선, GLM은 열 우선이므로 전치하면서 복사
        to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
        to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
        to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
        to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
        return to;
    }

    /*
     * GetGLMVec - Assimp 3D 벡터를 GLM 벡터로 변환
     * 
     * @param vec Assimp 3D 벡터
     * @return GLM 3D 벡터
     */
    inline glm::vec3 GetGLMVec(const aiVector3D& vec)
    {
        return glm::vec3(vec.x, vec.y, vec.z);
    }

    /*
     * GetGLMQuat - Assimp 쿼터니언을 GLM 쿼터니언으로 변환
     * 
     * 쿼터니언(Quaternion)이란?
     * - 3D 회전을 표현하는 방법입니다.
     * - 오일러 각도(x, y, z 회전)보다 짐벌 락 문제가 없어서 애니메이션에 적합합니다.
     * - w, x, y, z 네 개의 값으로 이루어집니다.
     * 
     * @param pOrientation Assimp 쿼터니언
     * @return GLM 쿼터니언
     */
    inline glm::quat GetGLMQuat(const aiQuaternion& pOrientation)
    {
        return glm::quat(pOrientation.w, pOrientation.x, pOrientation.y, pOrientation.z);
    }
}
