/*
 * Player.h
 * 
 * 플레이어 캐릭터 클래스입니다.
 * CharacterBase를 상속받아 플레이어만의 고유한 동작을 추가합니다.
 * 
 * 플레이어는 다음과 같은 공격을 할 수 있습니다:
 * - Attack: 기본 공격
 * - Slash: 베기 공격
 * - Cast: 마법 시전
 */

#ifndef PLAYER_H
#define PLAYER_H

#include <functional>
#include "CharacterBase.h"
#include "Enemy.h"

class Player : public CharacterBase
{
public:
	// 생성자: 이름 "Player", HP 100으로 초기화
	Player() : CharacterBase("Player", 100) {}

	/*
	 * Init - 플레이어 초기화
	 * 
	 * 모델과 애니메이션을 로드하고 초기 설정을 합니다.
	 * 
	 * @return 성공하면 true
	 */
	bool Init()
	{
		// 모델 로드
		if (!LoadModel("assets/models/Player/IdlePlayer.fbx")) return false;
		
		// 애니메이션 등록
		if (!AddAnimation("idle", "assets/models/Player/IdlePlayer.fbx")) return false;
		if (!AddAnimation("attack", "assets/models/Player/AttackPlayer.fbx")) return false;
		if (!AddAnimation("slash", "assets/models/Player/SlashPlayer.fbx")) return false;
		if (!AddAnimation("cast", "assets/models/Player/CastingPlayer.fbx")) return false;

		// 기본 상태는 idle
		PlayAnimation("idle");
		
		// 각 액션의 지속 시간 저장
		attack_.duration = GetDuration("attack");
		slash_.duration = GetDuration("slash");
		cast_.duration  = GetDuration("cast");
		return true;
	}

	/*
	 * UpdateActions - 매 프레임 업데이트
	 * 
	 * 애니메이션 타이머를 업데이트하고, 끝나면 idle로 복귀합니다.
	 * 
	 * @param dt 델타 타임 (초 단위)
	 */
	void UpdateActions(float dt)
	{
		// 액션 업데이트용 람다 함수
		auto update = [&](Action& a)
		{
			if (!a.playing) return;
			a.timer += dt;
			if (a.timer >= a.duration)
			{
				// 액션이 끝나면 idle로 복귀
				PlayAnimation("idle");
				LogAnimChange("Player", "idle", animState_);
				a.playing = false;
				
				// 액션 종료 콜백 호출 (적 스턴 트리거 등)
				if (onActionFinished_) onActionFinished_();
			}
		};

		Update(dt);
		update(attack_);
		update(slash_);
		update(cast_);
	}

	// 공격 시작 함수들
	bool StartAttack() { return StartAction(attack_); }
	bool StartSlash()  { return StartAction(slash_); }
	bool StartCast()   { return StartAction(cast_); }

	// 현재 어떤 액션이 재생 중인지 확인
	bool IsAnyPlaying() const { return attack_.playing || slash_.playing || cast_.playing; }

	/*
	 * SetOnActionFinished - 액션 종료 시 콜백 설정
	 * 
	 * 공격이 끝났을 때 호출될 함수를 설정합니다.
	 * 예: 플레이어 공격이 끝나면 적이 스턴됨
	 */
	void SetOnActionFinished(std::function<void()> cb) { onActionFinished_ = std::move(cb); }

	// 공격 대상(적) 설정
	void SetAttackTarget(Enemy* target) { attackTarget_ = target; }

	/*
	 * Action - 액션 정보 구조체
	 */
	struct Action
	{
		std::string key;       // 애니메이션 키
		int damage = 10;       // 데미지 양
		float duration = 1.0f; // 지속 시간
		float timer = 0.0f;    // 현재 경과 시간
		bool playing = false;  // 재생 중인지
	};

private:
	/*
	 * StartAction - 액션 시작
	 * 
	 * 공통 액션 시작 로직입니다.
	 * 
	 * @param a 시작할 액션
	 * @return 성공하면 true
	 */
	bool StartAction(Action& a)
	{
		// 적이 없거나 이미 죽었으면 공격 불가
		if (!attackTarget_ || !attackTarget_->IsAlive()) return false;
		if (a.playing) return false;
		if (!PlayAnimation(a.key)) return false;

		LogAnimChange("Player", a.key.c_str(), animState_);
		LogAnimStart("Player", a.key.c_str(), a.duration);

		// 적에게 데미지
		std::string reason = std::string("Player ") + a.key;
		attackTarget_->TakeDamage(a.damage, reason.c_str());

		// 다른 액션 중단하고 이 액션 시작
		attack_.playing = slash_.playing = cast_.playing = false;
		a.playing = true;
		a.timer = 0.0f;
		return true;
	}

	// 각 액션 정의
	Action attack_{ "attack", 10 };
	Action slash_{ "slash", 10 };
	Action cast_{ "cast", 10 };
	
	std::function<void()> onActionFinished_;  // 액션 종료 시 콜백
	std::string animState_ = "idle";          // 현재 상태 (로그용)
	Enemy* attackTarget_ = nullptr;           // 공격 대상
};

#endif // PLAYER_H
