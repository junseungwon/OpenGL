/*
 * shader_m.h
 * 
 * 셰이더 프로그램을 관리하는 클래스입니다.
 * 
 * 셰이더란?
 * - GPU에서 실행되는 작은 프로그램입니다.
 * - 버텍스 셰이더: 정점의 위치를 계산합니다 (3D → 2D 변환)
 * - 프래그먼트 셰이더: 각 픽셀의 색상을 계산합니다 (조명, 텍스처 등)
 */

#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <glm.hpp>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader
{
public:
    unsigned int ID;  // 셰이더 프로그램 ID

    /*
     * 생성자 - 셰이더 파일을 읽어서 컴파일하고 링크합니다
     * 
     * @param vertexPath   버텍스 셰이더 파일 경로 (.vs)
     * @param fragmentPath 프래그먼트 셰이더 파일 경로 (.fs)
     */
    Shader(const char* vertexPath, const char* fragmentPath)
    {
        // 1단계: 파일에서 셰이더 소스 코드 읽기
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;

        // 파일 읽기 실패 시 예외 발생하도록 설정
        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        try
        {
            // 파일 열기
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;

            // 파일 내용을 스트림으로 읽기
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();

            // 파일 닫기
            vShaderFile.close();
            fShaderFile.close();

            // 스트림을 문자열로 변환
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
        }
        catch (std::ifstream::failure& e)
        {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
        }

        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();

        // 2단계: 셰이더 컴파일
        unsigned int vertex, fragment;

        // 버텍스 셰이더 컴파일
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");

        // 프래그먼트 셰이더 컴파일
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");

        // 3단계: 셰이더 프로그램 링크 (버텍스 + 프래그먼트를 하나로 연결)
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");

        // 개별 셰이더 삭제 (프로그램에 연결되었으므로 더 이상 필요 없음)
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    /*
     * use - 이 셰이더 프로그램을 사용하도록 설정
     * 
     * 이후의 그리기 명령은 이 셰이더를 사용합니다.
     */
    void use() const
    {
        glUseProgram(ID);
    }

    // ========== 유니폼 변수 설정 함수들 ==========
    // 유니폼: CPU에서 GPU(셰이더)로 데이터를 전달하는 방법

    // bool 값 전달
    void setBool(const std::string& name, bool value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }

    // int 값 전달
    void setInt(const std::string& name, int value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }

    // float 값 전달
    void setFloat(const std::string& name, float value) const
    {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }

    // 2D 벡터 전달
    void setVec2(const std::string& name, const glm::vec2& value) const
    {
        glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec2(const std::string& name, float x, float y) const
    {
        glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
    }

    // 3D 벡터 전달 (위치, 색상 등에 사용)
    void setVec3(const std::string& name, const glm::vec3& value) const
    {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec3(const std::string& name, float x, float y, float z) const
    {
        glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
    }

    // 4D 벡터 전달
    void setVec4(const std::string& name, const glm::vec4& value) const
    {
        glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec4(const std::string& name, float x, float y, float z, float w) const
    {
        glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
    }

    // 2x2 행렬 전달
    void setMat2(const std::string& name, const glm::mat2& mat) const
    {
        glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    // 3x3 행렬 전달
    void setMat3(const std::string& name, const glm::mat3& mat) const
    {
        glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    // 4x4 행렬 전달 (변환 행렬에 주로 사용)
    void setMat4(const std::string& name, const glm::mat4& mat) const
    {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

private:
    /*
     * checkCompileErrors - 셰이더 컴파일/링크 에러 확인
     * 
     * @param shader 확인할 셰이더 또는 프로그램 ID
     * @param type   "VERTEX", "FRAGMENT", 또는 "PROGRAM"
     */
    void checkCompileErrors(GLuint shader, std::string type)
    {
        GLint success;
        GLchar infoLog[1024];

        if (type != "PROGRAM")
        {
            // 셰이더 컴파일 에러 확인
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << std::endl;
            }
        }
        else
        {
            // 프로그램 링크 에러 확인
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << std::endl;
            }
        }
    }
};

#endif
