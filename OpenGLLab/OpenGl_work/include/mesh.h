/*
 * mesh.h
 * 
 * 3D 모델의 기본 단위인 메시(Mesh)를 정의합니다.
 * 메시는 정점(꼭짓점)들과 텍스처로 이루어진 3D 도형입니다.
 * 예를 들어 캐릭터 모델은 얼굴 메시, 몸통 메시, 팔 메시 등 여러 메시로 구성됩니다.
 */

#ifndef MESH_H
#define MESH_H

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#include <glad/glad.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <shader_m.h>
#include <string>
#include <vector>
using namespace std;

// 하나의 정점에 영향을 줄 수 있는 최대 뼈대 개수
// 스킨 애니메이션에서 정점이 여러 뼈대의 영향을 받을 수 있음
#define MAX_BONE_INFLUENCE 4

/*
 * Vertex - 정점 구조체
 * 
 * 3D 모델의 각 꼭짓점이 가지는 정보들입니다.
 * 정점 하나하나가 이 정보를 가지고 있어서 GPU가 화면에 그릴 수 있습니다.
 */
struct Vertex {
    glm::vec3 Position;    // 위치 (x, y, z 좌표)
    glm::vec3 Normal;      // 법선 벡터 (조명 계산에 사용, 면이 바라보는 방향)
    glm::vec2 TexCoords;   // 텍스처 좌표 (이미지의 어느 부분을 사용할지)
    glm::vec3 Tangent;     // 탄젠트 벡터 (노멀맵 계산에 사용)
    glm::vec3 Bitangent;   // 바이탄젠트 벡터 (노멀맵 계산에 사용)
    
    // 스켈레탈 애니메이션용 데이터
    int m_BoneIDs[MAX_BONE_INFLUENCE];    // 이 정점에 영향을 주는 뼈대 ID들
    float m_Weights[MAX_BONE_INFLUENCE];  // 각 뼈대가 이 정점에 미치는 영향력 (0~1)
};

/*
 * Texture - 텍스처 구조체
 * 
 * 3D 모델에 입히는 이미지(텍스처) 정보입니다.
 */
struct Texture {
    Texture() : id(0) {}   // 기본 생성자
    unsigned int id;       // OpenGL 텍스처 ID
    string type;           // 텍스처 종류 ("texture_diffuse", "texture_specular" 등)
    string path;           // 텍스처 파일 경로
};

/*
 * Mesh - 메시 클래스
 * 
 * 정점들과 텍스처를 조합해서 하나의 3D 도형을 만드는 클래스입니다.
 * OpenGL 버퍼에 데이터를 올려서 GPU가 그릴 수 있게 합니다.
 */
class Mesh {
public:
    // 메시 데이터
    vector<Vertex>       vertices;  // 정점 배열
    vector<unsigned int> indices;   // 인덱스 배열 (삼각형을 어떤 정점으로 만들지)
    vector<Texture>      textures;  // 텍스처 배열
    unsigned int VAO;               // Vertex Array Object (정점 데이터 묶음)

    /*
     * 생성자 - 메시 데이터를 받아서 GPU에 업로드합니다
     */
    Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures)
    {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;
        setupMesh();  // GPU 버퍼 설정
    }

    /*
     * Draw - 메시를 화면에 그립니다
     * 
     * @param shader 사용할 셰이더 프로그램
     */
    void Draw(Shader &shader) 
    {
        // 텍스처 바인딩 (셰이더에서 사용할 수 있게 연결)
        unsigned int diffuseNr  = 1;  // 디퓨즈 텍스처 번호
        unsigned int specularNr = 1;  // 스페큘러 텍스처 번호
        unsigned int normalNr   = 1;  // 노멀맵 텍스처 번호
        unsigned int heightNr   = 1;  // 높이맵 텍스처 번호

        for(unsigned int i = 0; i < textures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i);  // 텍스처 유닛 활성화
            
            // 텍스처 종류에 따라 번호 부여
            string number;
            string name = textures[i].type;
            if(name == "texture_diffuse")
                number = std::to_string(diffuseNr++);
            else if(name == "texture_specular")
                number = std::to_string(specularNr++);
            else if(name == "texture_normal")
                number = std::to_string(normalNr++);
            else if(name == "texture_height")
                number = std::to_string(heightNr++);

            // 셰이더에 텍스처 유닛 번호 전달
            glUniform1i(glGetUniformLocation(shader.ID, (name + number).c_str()), i);
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }
        
        // 메시 그리기
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glActiveTexture(GL_TEXTURE0);  // 기본 텍스처 유닛으로 복원
    }

private:
    unsigned int VBO, EBO;  // Vertex Buffer Object, Element Buffer Object

    /*
     * setupMesh - GPU 버퍼를 설정합니다
     * 
     * 정점 데이터를 GPU 메모리에 올리고,
     * 셰이더가 데이터를 어떻게 읽을지 설정합니다.
     */
    void setupMesh()
    {
        // VAO, VBO, EBO 생성
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        // 정점 데이터를 VBO에 업로드
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);  

        // 인덱스 데이터를 EBO에 업로드
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // 정점 속성 설정 (셰이더가 데이터를 어떻게 읽을지)
        
        // 위치 (layout = 0)
        glEnableVertexAttribArray(0);	
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        
        // 법선 (layout = 1)
        glEnableVertexAttribArray(1);	
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        
        // 텍스처 좌표 (layout = 2)
        glEnableVertexAttribArray(2);	
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
        
        // 탄젠트 (layout = 3)
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
        
        // 바이탄젠트 (layout = 4)
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));
        
        // 뼈대 ID (layout = 5) - 정수형이라 glVertexAttribIPointer 사용
        glEnableVertexAttribArray(5);
        glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, m_BoneIDs));

        // 뼈대 가중치 (layout = 6)
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_Weights));
        
        glBindVertexArray(0);
    }
};

#endif
