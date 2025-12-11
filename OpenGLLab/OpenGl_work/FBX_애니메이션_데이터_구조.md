# 📦 FBX 파일 안의 애니메이션 데이터 구조

FBX 파일에 들어있는 애니메이션 데이터가 어떤 것들인지 자세히 설명합니다.

---

## 📋 전체 구조

```
FBX 파일
└─ aiScene (씬 전체)
    └─ aiAnimation[] (애니메이션 배열)
        └─ aiAnimation (하나의 애니메이션 클립)
            ├─ mDuration (애니메이션 길이)
            ├─ mTicksPerSecond (초당 틱 수)
            └─ mChannels[] (뼈대별 애니메이션 채널)
                └─ aiNodeAnim (하나의 뼈대 애니메이션)
                    ├─ mNodeName (뼈대 이름)
                    ├─ mPositionKeys[] (위치 키프레임)
                    ├─ mRotationKeys[] (회전 키프레임)
                    └─ mScalingKeys[] (크기 키프레임)
```

---

## 1. aiAnimation (애니메이션 클립 전체 정보)

**하나의 애니메이션 클립**을 나타냅니다. 예: "걷기", "공격", "뛰기"

### 데이터 구조

```cpp
// Assimp의 aiAnimation 구조체
struct aiAnimation {
    aiString mName;              // 애니메이션 이름 (예: "Walk", "Attack")
    double mDuration;            // 애니메이션 길이 (틱 단위)
    double mTicksPerSecond;      // 초당 틱 수 (예: 30, 60)
    unsigned int mNumChannels;   // 애니메이션 채널 개수 (뼈대 개수)
    aiNodeAnim** mChannels;      // 각 뼈대의 애니메이션 데이터 배열
};
```

### 코드에서 읽는 방법

```cpp
// animation.cpp
const aiAnimation* animation = scene->mAnimations[0];  // 첫 번째 애니메이션

m_Duration = (float)animation->mDuration;        // 예: 120 틱
m_TicksPerSecond = animation->mTicksPerSecond;  // 예: 30 (초당 30틱)
// → 실제 길이 = 120 / 30 = 4초

// 각 뼈대의 애니메이션 데이터 읽기
for (int i = 0; i < animation->mNumChannels; i++) {
    auto channel = animation->mChannels[i];  // aiNodeAnim
    // channel 처리...
}
```

### 실제 예시

```
애니메이션: "Attack"
- Duration: 90 틱
- TicksPerSecond: 30
- 실제 길이: 90 / 30 = 3초
- Channels: 50개 (50개 뼈대가 애니메이션됨)
```

---

## 2. aiNodeAnim (뼈대별 애니메이션 채널)

**하나의 뼈대**가 시간에 따라 어떻게 변하는지 저장합니다.

### 데이터 구조

```cpp
// Assimp의 aiNodeAnim 구조체
struct aiNodeAnim {
    aiString mNodeName;              // 뼈대 이름 (예: "UpperArm", "Spine")
    
    unsigned int mNumPositionKeys;   // 위치 키프레임 개수
    aiVectorKey* mPositionKeys;      // 위치 키프레임 배열
    
    unsigned int mNumRotationKeys;   // 회전 키프레임 개수
    aiQuatKey* mRotationKeys;        // 회전 키프레임 배열
    
    unsigned int mNumScalingKeys;    // 크기 키프레임 개수
    aiVectorKey* mScalingKeys;       // 크기 키프레임 배열
};
```

### 코드에서 읽는 방법

```cpp
// bone.cpp - Bone 생성자
Bone::Bone(const std::string& name, int id, const aiNodeAnim* channel)
{
    // 1. 위치 키프레임 읽기
    m_NumPositions = channel->mNumPositionKeys;
    for (int i = 0; i < m_NumPositions; ++i) {
        aiVector3D aiPosition = channel->mPositionKeys[i].mValue;  // 위치 (x, y, z)
        float timeStamp = (float)channel->mPositionKeys[i].mTime;   // 시간 (틱)
        
        KeyPosition data;
        data.position = AssimpGLMHelpers::GetGLMVec(aiPosition);
        data.timeStamp = timeStamp;
        m_Positions.push_back(data);
    }
    
    // 2. 회전 키프레임 읽기
    m_NumRotations = channel->mNumRotationKeys;
    for (int i = 0; i < m_NumRotations; ++i) {
        aiQuaternion aiOrientation = channel->mRotationKeys[i].mValue;  // 회전 (쿼터니언)
        float timeStamp = (float)channel->mRotationKeys[i].mTime;        // 시간 (틱)
        
        KeyRotation data;
        data.orientation = AssimpGLMHelpers::GetGLMQuat(aiOrientation);
        data.timeStamp = timeStamp;
        m_Rotations.push_back(data);
    }
    
    // 3. 크기 키프레임 읽기
    m_NumScales = channel->mNumScalingKeys;
    for (int i = 0; i < m_NumScales; ++i) {
        aiVector3D scale = channel->mScalingKeys[i].mValue;  // 크기 (x, y, z 배율)
        float timeStamp = (float)channel->mScalingKeys[i].mTime;  // 시간 (틱)
        
        KeyScale data;
        data.scale = AssimpGLMHelpers::GetGLMVec(scale);
        data.timeStamp = timeStamp;
        m_Scales.push_back(data);
    }
}
```

---

## 3. 키프레임 데이터 상세

### 3-1. 위치 키프레임 (Position Keys)

**뼈대가 특정 시간에 어디에 있는지** 저장합니다.

```cpp
// aiVectorKey 구조체
struct aiVectorKey {
    double mTime;        // 시간 (틱 단위)
    aiVector3D mValue;   // 위치 (x, y, z)
};
```

**실제 예시: UpperArm 뼈대**

```
키프레임 0: 시간 0틱, 위치 (0, 1, 0)     → 시작 위치
키프레임 1: 시간 30틱, 위치 (0, 1.5, 0)  → 위로 올라감
키프레임 2: 시간 60틱, 위치 (0, 1, 0)    → 다시 내려옴
키프레임 3: 시간 90틱, 위치 (0, 1, 0)    → 원래 위치
```

**코드에서 사용:**

```cpp
// bone.cpp - InterpolatePosition()
glm::mat4 Bone::InterpolatePosition(float animationTime)
{
    // 현재 시간이 어느 키프레임 사이에 있는지 찾기
    int p0Index = GetPositionIndex(animationTime);  // 예: 1번 키프레임
    int p1Index = p0Index + 1;                      // 예: 2번 키프레임
    
    // 보간 비율 계산
    float scaleFactor = GetScaleFactor(
        m_Positions[p0Index].timeStamp,  // 30틱
        m_Positions[p1Index].timeStamp,  // 60틱
        animationTime                     // 예: 45틱
    );
    // scaleFactor = (45 - 30) / (60 - 30) = 0.5
    
    // 선형 보간
    glm::vec3 finalPosition = glm::mix(
        m_Positions[p0Index].position,  // (0, 1.5, 0)
        m_Positions[p1Index].position,  // (0, 1, 0)
        scaleFactor                      // 0.5
    );
    // 결과: (0, 1.25, 0) - 중간 위치
    
    return glm::translate(glm::mat4(1.0f), finalPosition);
}
```

### 3-2. 회전 키프레임 (Rotation Keys)

**뼈대가 특정 시간에 어떻게 회전되어 있는지** 저장합니다.

```cpp
// aiQuatKey 구조체
struct aiQuatKey {
    double mTime;        // 시간 (틱 단위)
    aiQuaternion mValue; // 회전 (쿼터니언: x, y, z, w)
};
```

**실제 예시: UpperArm 뼈대**

```
키프레임 0: 시간 0틱, 회전 (0, 0, 0, 1)      → 0도 (원래 자세)
키프레임 1: 시간 30틱, 회전 (0.707, 0, 0, 0.707) → 90도 회전
키프레임 2: 시간 60틱, 회전 (0, 0, 0, 1)      → 0도 (원래 자세)
```

**쿼터니언이란?**
- 3D 회전을 표현하는 수학적 방법
- 오일러 각도보다 부드럽고 짐벌 락 문제가 없음
- (x, y, z, w) 4개의 값으로 회전 표현

**코드에서 사용:**

```cpp
// bone.cpp - InterpolateRotation()
glm::mat4 Bone::InterpolateRotation(float animationTime)
{
    // 현재 시간이 어느 키프레임 사이에 있는지 찾기
    int p0Index = GetRotationIndex(animationTime);
    int p1Index = p0Index + 1;
    
    // 보간 비율 계산
    float scaleFactor = GetScaleFactor(
        m_Rotations[p0Index].timeStamp,
        m_Rotations[p1Index].timeStamp,
        animationTime
    );
    
    // 구면 선형 보간 (SLERP) - 회전은 쿼터니언으로
    glm::quat finalRotation = glm::slerp(
        m_Rotations[p0Index].orientation,  // 시작 회전
        m_Rotations[p1Index].orientation,  // 끝 회전
        scaleFactor                         // 보간 비율
    );
    
    return glm::toMat4(finalRotation);  // 쿼터니언 → 행렬 변환
}
```

### 3-3. 크기 키프레임 (Scaling Keys)

**뼈대가 특정 시간에 얼마나 크기가 변하는지** 저장합니다.

```cpp
// aiVectorKey 구조체 (위치와 동일한 구조)
struct aiVectorKey {
    double mTime;        // 시간 (틱 단위)
    aiVector3D mValue;   // 크기 (x, y, z 배율)
};
```

**실제 예시: 손 뼈대 (주먹 쥐기)**

```
키프레임 0: 시간 0틱, 크기 (1, 1, 1)      → 원래 크기
키프레임 1: 시간 15틱, 크기 (0.8, 0.8, 0.8) → 작아짐 (주먹 쥠)
키프레임 2: 시간 30틱, 크기 (1, 1, 1)      → 원래 크기
```

**코드에서 사용:**

```cpp
// bone.cpp - InterpolateScaling()
glm::mat4 Bone::InterpolateScaling(float animationTime)
{
    // 현재 시간이 어느 키프레임 사이에 있는지 찾기
    int p0Index = GetScaleIndex(animationTime);
    int p1Index = p0Index + 1;
    
    // 보간 비율 계산
    float scaleFactor = GetScaleFactor(
        m_Scales[p0Index].timeStamp,
        m_Scales[p1Index].timeStamp,
        animationTime
    );
    
    // 선형 보간
    glm::vec3 finalScale = glm::mix(
        m_Scales[p0Index].scale,  // 시작 크기
        m_Scales[p1Index].scale,  // 끝 크기
        scaleFactor                // 보간 비율
    );
    
    return glm::scale(glm::mat4(1.0f), finalScale);
}
```

---

## 4. 전체 데이터 흐름

### FBX 파일 → 코드 → 사용

```
[FBX 파일]
└─ "Attack" 애니메이션
    ├─ Duration: 90틱, TicksPerSecond: 30
    └─ Channels (50개 뼈대)
        ├─ "UpperArm" 채널
        │   ├─ PositionKeys: 4개
        │   │   ├─ (0틱, 0,1,0)
        │   │   ├─ (30틱, 0,1.5,0)
        │   │   ├─ (60틱, 0,1,0)
        │   │   └─ (90틱, 0,1,0)
        │   ├─ RotationKeys: 3개
        │   │   ├─ (0틱, 0도)
        │   │   ├─ (30틱, 90도)
        │   │   └─ (60틱, 0도)
        │   └─ ScalingKeys: 1개
        │       └─ (0틱, 1,1,1)
        ├─ "LowerArm" 채널
        │   └─ ...
        └─ ... (나머지 48개 뼈대)
            │
            ▼
[코드에서 읽기]
└─ Animation 클래스
    ├─ m_Duration = 90
    ├─ m_TicksPerSecond = 30
    └─ m_Bones[] (50개)
        ├─ Bone("UpperArm", ...)
        │   ├─ m_Positions[] (4개)
        │   ├─ m_Rotations[] (3개)
        │   └─ m_Scales[] (1개)
        └─ ... (나머지 49개)
            │
            ▼
[실시간 사용]
└─ Animator::UpdateAnimation(dt)
    └─ 현재 시간: 45틱
        └─ 각 뼈대의 Bone::Update(45틱) 호출
            ├─ UpperArm::InterpolatePosition(45틱)
            │   └─ 키프레임 1과 2 사이 보간
            │       └─ 결과: (0, 1.25, 0)
            ├─ UpperArm::InterpolateRotation(45틱)
            │   └─ 키프레임 1과 2 사이 보간
            │       └─ 결과: 45도 회전
            └─ UpperArm::InterpolateScaling(45틱)
                └─ 결과: (1, 1, 1)
                    │
                    ▼
[최종 변환 행렬]
└─ m_LocalTransform = translation × rotation × scale
    └─ 셰이더로 전달 → 정점 변환
```

---

## 5. 실제 데이터 예시

### 예시: "Attack" 애니메이션 (3초, 30fps)

```
애니메이션 정보:
- 이름: "Attack"
- 길이: 90틱
- 초당 틱: 30
- 실제 길이: 3초
- 뼈대 개수: 50개

UpperArm 뼈대:
위치 키프레임:
  0틱:  (0.0, 1.0, 0.0)   시작 위치
  30틱: (0.0, 1.5, 0.0)  위로 올라감
  60틱: (0.0, 1.0, 0.0)  다시 내려옴
  90틱: (0.0, 1.0, 0.0)  원래 위치

회전 키프레임:
  0틱:  0도 (원래 자세)
  30틱: 90도 (팔 올림)
  60틱: 0도 (원래 자세)

크기 키프레임:
  0틱:  (1.0, 1.0, 1.0)  원래 크기
```

---

## 6. 키프레임 보간 (Interpolation)

### 왜 보간이 필요한가?

키프레임은 **중요한 시점만** 저장합니다:
- 0초: 팔 내림
- 1초: 팔 올림
- 2초: 팔 내림

하지만 **0.5초, 1.5초** 같은 중간 시간도 필요합니다!

### 보간 방법

**선형 보간 (Lerp)**: 위치, 크기
```
시작값 + (끝값 - 시작값) × 비율

예: 0초 위치 (0, 1, 0), 1초 위치 (0, 2, 0)
    0.5초 위치 = (0, 1, 0) + ((0, 2, 0) - (0, 1, 0)) × 0.5
                = (0, 1.5, 0)
```

**구면 선형 보간 (SLERP)**: 회전
```
쿼터니언을 구면에서 부드럽게 보간

예: 0도 → 90도
    0.5초 = 45도 (부드러운 회전)
```

---

## 7. 요약

### FBX 파일에 들어있는 애니메이션 데이터

1. **애니메이션 전체 정보**
   - 이름
   - 길이 (틱 단위)
   - 초당 틱 수

2. **각 뼈대별 데이터** (채널)
   - 뼈대 이름
   - 위치 키프레임 배열 (시간, 위치)
   - 회전 키프레임 배열 (시간, 회전)
   - 크기 키프레임 배열 (시간, 크기)

3. **키프레임 데이터**
   - 시간 (틱 단위)
   - 값 (위치/회전/크기)

### 데이터 사용 흐름

```
FBX 파일 읽기
  ↓
키프레임 데이터 추출
  ↓
Bone 객체에 저장
  ↓
실시간으로 보간 계산
  ↓
변환 행렬 생성
  ↓
셰이더로 전달
  ↓
정점 변환
```

---

이제 FBX 파일 안에 어떤 애니메이션 데이터가 들어있는지 완전히 이해하셨을 것입니다! 🎉

