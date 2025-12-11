/*
 * model.h
 * 
 * 3D 모델 파일(FBX, OBJ 등)을 로드하고 관리하는 클래스입니다.
 * 
 * Model이란?
 * - 여러 개의 메시(Mesh)로 이루어진 3D 오브젝트입니다.
 * - 예: 캐릭터 모델 = 얼굴 메시 + 몸통 메시 + 팔 메시 + ...
 * - 각 메시에는 텍스처(이미지)가 입혀져 있습니다.
 * 
 * Assimp 라이브러리를 사용해서 다양한 3D 파일 형식을 읽어옵니다.
 */

#ifndef MODEL_H
#define MODEL_H

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#include <glad/glad.h> 
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <stb_image.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <mesh.h>
#include <shader_m.h>
#include <animdata.h>
#include <assimp_glm_helpers.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
using namespace std;

// 텍스처 파일 로드 함수 (파일에서 이미지를 읽어 OpenGL 텍스처로 변환)
unsigned int TextureFromFile(const char *path, const string &directory, bool gamma = false);

/*
 * Model - 3D 모델 클래스
 * 
 * FBX, OBJ 등의 3D 파일을 로드하고 화면에 그립니다.
 * 스켈레탈 애니메이션을 위한 뼈대 정보도 추출합니다.
 */
class Model 
{
public:
    vector<Texture> textures_loaded;  // 로드된 텍스처들 (중복 로드 방지용)
    vector<Mesh> meshes;              // 메시 목록
    string directory;                 // 모델 파일이 있는 폴더
    bool gammaCorrection;             // 감마 보정 여부

    map<string, BoneInfo> m_BoneInfoMap;  // 뼈대 이름 → 정보 맵
    int m_BoneCounter = 0;                // 뼈대 개수

    /*
     * 생성자 - 모델 파일 로드
     * @param path 모델 파일 경로 (예: "assets/models/character.fbx")
     * @param gamma 감마 보정 활성화 여부
     */
    Model(string const &path, bool gamma = false) : gammaCorrection(gamma)
    {
        loadModel(path);
    }

    /*
     * Draw - 모델의 모든 메시를 그립니다
     * @param shader 사용할 셰이더
     */
    void Draw(Shader &shader)
    {
        for(unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);
    }

    // 뼈대 정보 맵 반환
    map<string, BoneInfo>& GetBoneInfoMap() { return m_BoneInfoMap; }
    
    // 뼈대 개수 반환
    int& GetBoneCount() { return m_BoneCounter; }
    
private:
    /*
     * loadModel - Assimp으로 모델 파일 로드
     */
    void loadModel(string const &path)
    {
        Assimp::Importer importer;
        
        // 모델 파일 읽기 (삼각형화, 노멀 생성, UV 뒤집기, 탄젠트 계산)
        const aiScene* scene = importer.ReadFile(path, 
            aiProcess_Triangulate | aiProcess_GenSmoothNormals | 
            aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
        
        // 에러 체크
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
            return;
        }
        
        // 디렉토리 경로 추출
        directory = path.substr(0, path.find_last_of('/'));
        
        // 노드 트리 순회하며 메시 처리
        processNode(scene->mRootNode, scene);
    }

    /*
     * processNode - 노드 트리를 재귀적으로 처리
     * 
     * 3D 모델은 트리 구조로 되어 있습니다.
     * 각 노드를 방문하면서 메시를 추출합니다.
     */
    void processNode(aiNode *node, const aiScene *scene)
    {
        // 현재 노드의 메시들 처리
        for(unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        
        // 자식 노드들 재귀 처리
        for(unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }
    }

    /*
     * processMesh - 하나의 메시 처리
     * 
     * Assimp 메시에서 정점, 인덱스, 텍스처, 뼈대 정보를 추출합니다.
     */
    Mesh processMesh(aiMesh *mesh, const aiScene *scene)
    {
        vector<Vertex> vertices;
        vector<unsigned int> indices;
        vector<Texture> textures;

        // 정점 데이터 추출
        for(unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            SetVertexBoneDataToDefault(vertex);
            
            // 위치
            vertex.Position = glm::vec3(
                mesh->mVertices[i].x,
                mesh->mVertices[i].y,
                mesh->mVertices[i].z
            );
            
            // 법선
            if (mesh->HasNormals())
            {
                vertex.Normal = glm::vec3(
                    mesh->mNormals[i].x,
                    mesh->mNormals[i].y,
                    mesh->mNormals[i].z
                );
            }
            
            // 텍스처 좌표
            if(mesh->mTextureCoords[0])
            {
                vertex.TexCoords = glm::vec2(
                    mesh->mTextureCoords[0][i].x, 
                    mesh->mTextureCoords[0][i].y
                );
                
                // 탄젠트, 바이탄젠트
                vertex.Tangent = glm::vec3(
                    mesh->mTangents[i].x,
                    mesh->mTangents[i].y,
                    mesh->mTangents[i].z
                );
                vertex.Bitangent = glm::vec3(
                    mesh->mBitangents[i].x,
                    mesh->mBitangents[i].y,
                    mesh->mBitangents[i].z
                );
            }
            else
            {
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            }

            vertices.push_back(vertex);
        }
        
        // 뼈대 가중치 추출
        ExtractBoneWeightForVertices(vertices, mesh, scene);
        
        // 인덱스 추출 (삼각형을 이루는 정점 번호들)
        for(unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for(unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }
        
        // 텍스처 로드
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        
        // 디퓨즈 맵 (기본 색상 텍스처)
        vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        
        // 스페큘러 맵 (반사광 텍스처)
        vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        
        // 노멀 맵
        vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        
        // 높이 맵
        vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
        
        return Mesh(vertices, indices, textures);
    }

    /*
     * SetVertexBoneDataToDefault - 정점의 뼈대 데이터 초기화
     */
    void SetVertexBoneDataToDefault(Vertex& vertex)
    {
        for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
        {
            vertex.m_BoneIDs[i] = -1;    // -1은 영향 없음
            vertex.m_Weights[i] = 0.0f;
        }
    }

    /*
     * SetVertexBoneData - 정점에 뼈대 영향 추가
     */
    void SetVertexBoneData(Vertex& vertex, int boneID, float weight)
    {
        for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
        {
            if (vertex.m_BoneIDs[i] < 0)
            {
                vertex.m_BoneIDs[i] = boneID;
                vertex.m_Weights[i] = weight;
                return;
            }
        }
        // MAX_BONE_INFLUENCE 초과 시 무시
    }

    /*
     * ExtractBoneWeightForVertices - 뼈대 가중치 추출
     * 
     * 각 뼈대가 어떤 정점에 얼마나 영향을 주는지 추출합니다.
     */
    void ExtractBoneWeightForVertices(vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene)
    {
        for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
        {
            string boneName = mesh->mBones[boneIndex]->mName.C_Str();

            // 새 뼈대면 맵에 추가
            if (m_BoneInfoMap.find(boneName) == m_BoneInfoMap.end())
            {
                BoneInfo newBoneInfo;
                newBoneInfo.id = m_BoneCounter;
                newBoneInfo.offset = AssimpGLMHelpers::ConvertMatrixToGLMFormat(
                    mesh->mBones[boneIndex]->mOffsetMatrix);
                m_BoneInfoMap[boneName] = newBoneInfo;
                m_BoneCounter++;
            }

            int boneID = m_BoneInfoMap[boneName].id;
            auto weights = mesh->mBones[boneIndex]->mWeights;
            int numWeights = mesh->mBones[boneIndex]->mNumWeights;

            // 각 정점에 뼈대 영향 설정
            for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
            {
                int vertexId = weights[weightIndex].mVertexId;
                float weight = weights[weightIndex].mWeight;
                
                if (vertexId >= 0 && static_cast<size_t>(vertexId) < vertices.size())
                {
                    SetVertexBoneData(vertices[vertexId], boneID, weight);
                }
            }
        }
    }

    /*
     * loadMaterialTextures - 머티리얼의 텍스처 로드
     * 
     * 이미 로드된 텍스처는 재사용합니다.
     */
    vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName)
    {
        vector<Texture> textures;
        
        for(unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            
            // 이미 로드된 텍스처인지 확인
            bool skip = false;
            for(unsigned int j = 0; j < textures_loaded.size(); j++)
            {
                if(std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true;
                    break;
                }
            }
            
            if(!skip)
            {
                // 파일명 추출
                std::string requested = str.C_Str();
                size_t slash = requested.find_last_of("/\\");
                std::string fname = (slash == std::string::npos) ? requested : requested.substr(slash + 1);

                // 텍스처 로드 시도
                auto tryLoad = [&](const std::string& name) -> Texture {
                    Texture tex;
                    tex.id = TextureFromFile(name.c_str(), this->directory);
                    tex.type = typeName;
                    tex.path = name;
                    return tex;
                };

                Texture texture = tryLoad(fname);
                
                // 실패 시 대체 파일명 시도 (예: "file 1.png")
                if (texture.id == 0)
                {
                    std::string base = fname;
                    std::string ext = "";
                    size_t dot = fname.find_last_of('.');
                    if (dot != std::string::npos) {
                        base = fname.substr(0, dot);
                        ext = fname.substr(dot);
                    }
                    texture = tryLoad(base + " 1" + ext);
                    if (texture.id == 0)
                        texture = tryLoad(base + "_1" + ext);
                }

                if (texture.id != 0)
                {
                    textures.push_back(texture);
                    textures_loaded.push_back(texture);
                }
            }
        }
        return textures;
    }
};

/*
 * TextureFromFile - 파일에서 텍스처 로드
 * 
 * 이미지 파일을 읽어서 OpenGL 텍스처로 만듭니다.
 * 실패 시 1x1 흰색 텍스처를 대신 사용합니다.
 */
inline unsigned int TextureFromFile(const char *path, const string &directory, bool gamma)
{
    string filename = string(path);
    
    // 경로 구분자 통일
    std::replace(filename.begin(), filename.end(), '\\', '/');
    
    // 파일명만 추출
    size_t lastSlash = filename.find_last_of("/\\");
    if (lastSlash != string::npos)
        filename = filename.substr(lastSlash + 1);
    
    // 디렉토리와 결합
    filename = directory + '/' + filename;

    unsigned int textureID;
    glGenTextures(1, &textureID);

    // 텍스처 로드 시도
    auto tryLoad = [&](const std::string& pathToTry) -> bool {
        int width, height, nrComponents;
        unsigned char* data = stbi_load(pathToTry.c_str(), &width, &height, &nrComponents, 0);
        if (!data) return false;

        GLenum format;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;
        else format = GL_RGB;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        return true;
    };

    bool loaded = tryLoad(filename);

    // 대체 파일명 시도 (예: "file.png" → "file 1.png")
    if (!loaded)
    {
        size_t dot = filename.find_last_of('.');
        if (dot != std::string::npos)
        {
            std::string base = filename.substr(0, dot);
            std::string ext = filename.substr(dot);
            std::string alt = base + " 1" + ext;
            if (alt != filename)
                loaded = tryLoad(alt);
        }
    }

    // 실패 시 1x1 흰색 텍스처 생성
    if (!loaded)
    {
        unsigned char white[3] = { 255, 255, 255 };
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, white);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    return textureID;
}

#endif
