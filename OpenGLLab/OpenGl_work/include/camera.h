/*
 * camera.h
 * 
 * 3D 공간에서 카메라를 관리하는 클래스입니다.
 * 
 * 카메라란?
 * - 3D 세계를 어디서, 어느 방향으로 바라볼지 결정합니다.
 * - 우리 눈의 역할을 합니다.
 * - 카메라의 위치와 방향에 따라 화면에 보이는 장면이 달라집니다.
 */

#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

// 카메라 이동 방향 (키보드 입력에 사용)
enum Camera_Movement {
    FORWARD,   // 앞으로
    BACKWARD,  // 뒤로
    LEFT,      // 왼쪽
    RIGHT      // 오른쪽
};

// 카메라 기본값
const float YAW         = -90.0f;  // 좌우 회전 각도 (Y축 기준)
const float PITCH       =  0.0f;   // 상하 회전 각도 (X축 기준)
const float SPEED       =  2.5f;   // 이동 속도
const float SENSITIVITY =  0.1f;   // 마우스 감도
const float ZOOM        =  45.0f;  // 시야각 (Field of View)

/*
 * Camera - 카메라 클래스
 * 
 * 3D 공간에서 카메라의 위치, 방향, 이동을 관리합니다.
 * 오일러 각도(Yaw, Pitch)를 사용해서 카메라 방향을 계산합니다.
 */
class Camera
{
public:
    // ========== 카메라 속성 ==========
    glm::vec3 Position;  // 카메라 위치
    glm::vec3 Front;     // 카메라가 바라보는 방향
    glm::vec3 Up;        // 카메라의 위쪽 방향
    glm::vec3 Right;     // 카메라의 오른쪽 방향
    glm::vec3 WorldUp;   // 세계의 위쪽 방향 (보통 Y축)

    // 오일러 각도 (회전 각도)
    float Yaw;    // 좌우 회전 (수평면에서의 방향)
    float Pitch;  // 상하 회전 (고개를 위아래로)

    // 카메라 옵션
    float MovementSpeed;     // 이동 속도
    float MouseSensitivity;  // 마우스 감도
    float Zoom;              // 시야각 (작을수록 확대됨)

    /*
     * 생성자 - 벡터로 초기화
     * 
     * @param position 카메라 시작 위치
     * @param up       세계의 위쪽 방향
     * @param yaw      초기 좌우 회전 각도
     * @param pitch    초기 상하 회전 각도
     */
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), 
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), 
           float yaw = YAW, 
           float pitch = PITCH) 
        : Front(glm::vec3(0.0f, 0.0f, -1.0f)), 
          MovementSpeed(SPEED), 
          MouseSensitivity(SENSITIVITY), 
          Zoom(ZOOM)
    {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();  // 방향 벡터 계산
    }

    /*
     * 생성자 - 개별 값으로 초기화
     */
    Camera(float posX, float posY, float posZ, 
           float upX, float upY, float upZ, 
           float yaw, float pitch) 
        : Front(glm::vec3(0.0f, 0.0f, -1.0f)), 
          MovementSpeed(SPEED), 
          MouseSensitivity(SENSITIVITY), 
          Zoom(ZOOM)
    {
        Position = glm::vec3(posX, posY, posZ);
        WorldUp = glm::vec3(upX, upY, upZ);
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    /*
     * GetViewMatrix - 뷰 행렬 반환
     * 
     * 뷰 행렬은 3D 세계를 카메라 시점으로 변환하는 행렬입니다.
     * 셰이더에서 물체를 그릴 때 사용합니다.
     */
    glm::mat4 GetViewMatrix()
    {
        // lookAt: 카메라 위치, 바라보는 점, 위쪽 방향으로 뷰 행렬 생성
        return glm::lookAt(Position, Position + Front, Up);
    }

    /*
     * ProcessKeyboard - 키보드 입력 처리
     * 
     * WASD 키로 카메라를 이동시킵니다.
     * 
     * @param direction 이동 방향
     * @param deltaTime 프레임 간 시간 차이 (일정한 속도를 위해 필요)
     */
    void ProcessKeyboard(Camera_Movement direction, float deltaTime)
    {
        float velocity = MovementSpeed * deltaTime;  // 이동 거리

        if (direction == FORWARD)
            Position += Front * velocity;    // 앞으로
        if (direction == BACKWARD)
            Position -= Front * velocity;    // 뒤로
        if (direction == LEFT)
            Position -= Right * velocity;    // 왼쪽
        if (direction == RIGHT)
            Position += Right * velocity;    // 오른쪽
    }

    /*
     * ProcessMouseMovement - 마우스 이동 처리
     * 
     * 마우스를 움직이면 카메라가 회전합니다 (FPS 게임처럼).
     * 
     * @param xoffset 마우스 X축 이동량
     * @param yoffset 마우스 Y축 이동량
     * @param constrainPitch 상하 회전 제한 여부 (화면 뒤집힘 방지)
     */
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
    {
        // 마우스 감도 적용
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        // 회전 각도 업데이트
        Yaw   += xoffset;  // 좌우 회전
        Pitch += yoffset;  // 상하 회전

        // 상하 회전 제한 (화면이 뒤집히지 않게)
        if (constrainPitch)
        {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }

        // 새로운 각도로 방향 벡터 다시 계산
        updateCameraVectors();
    }

    /*
     * ProcessMouseScroll - 마우스 스크롤 처리
     * 
     * 스크롤로 줌인/줌아웃합니다.
     * 
     * @param yoffset 스크롤 양
     */
    void ProcessMouseScroll(float yoffset)
    {
        Zoom -= (float)yoffset;

        // 시야각 제한
        if (Zoom < 1.0f)
            Zoom = 1.0f;
        if (Zoom > 45.0f)
            Zoom = 45.0f;
    }

private:
    /*
     * updateCameraVectors - 카메라 방향 벡터 업데이트
     * 
     * Yaw와 Pitch 각도로부터 Front, Right, Up 벡터를 계산합니다.
     * 삼각함수를 사용해서 각도를 방향 벡터로 변환합니다.
     */
    void updateCameraVectors()
    {
        // 새로운 Front 벡터 계산
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);

        // Right와 Up 벡터도 다시 계산
        // 외적(cross)을 사용해서 수직인 벡터를 구함
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up    = glm::normalize(glm::cross(Right, Front));
    }
};

#endif
