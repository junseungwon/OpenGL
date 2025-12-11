/*
 * Enemy.h
 * 
 * 적(몬스터) 캐릭터 클래스입니다.
 * CharacterBase를 상속받아 적만의 고유한 동작을 추가합니다.
 * 
 * 적은 다음과 같은 상태를 가집니다:
 * - idle: 가만히 서있음
 * - attack: 공격
 * - stun: 플레이어에게 맞아서 기절
 * - punch: 기절에서 깨어나 반격
 * - dying: 사망
 */

#ifndef ENEMY_H
#define ENEMY_H

#include "CharacterBase.h"

class Enemy : public CharacterBase
{
public:
	// 생성자: 이름 "Enemy", HP 100으로 초기화
	Enemy() : CharacterBase("Enemy", 100) {}

	/*
	 * Init - 적 초기화
	 * 
	 * 모델과 애니메이션을 로드하고 초기 설정을 합니다.
	 * 게임 시작 시 한 번 호출합니다.
	 * 
	 * @return 성공하면 true, 실패하면 false
	 */
	bool Init()
	{
		// 모델 로드
		if (!LoadModel("assets/models/Eemy/IdleMonster.fbx")) return false;
		
		// 애니메이션 등록
		if (!AddAnimation("idle", "assets/models/Eemy/IdleMonster.fbx")) return false;
		if (!AddAnimation("attack", "assets/models/Eemy/AltAnim.fbx")) return false;
		if (!AddAnimation("stun", "assets/models/Eemy/Stun.fbx")) return false;
		if (!AddAnimation("punch", "assets/models/Eemy/Mutant Punch.fbx")) return false;
		if (!AddAnimation("dying", "assets/models/Eemy/Mutant Dying.fbx")) return false;

		// 기본 상태는 idle
		PlayAnimation("idle");

		// 각 애니메이션의 지속 시간 저장
		attack_.duration = GetDuration("attack");
		stun_.duration   = GetDuration("stun");
		punch_.duration  = GetDuration("punch");
		dying_.duration  = GetDuration("dying");

		// 너무 짧은 애니메이션은 최소 1초로 설정
		if (stun_.duration < 0.1f) stun_.duration = 1.0f;
		if (punch_.duration < 0.1f) punch_.duration = 1.0f;
		if (dying_.duration < 0.1f) dying_.duration = 1.0f;

		return true;
	}

	/*
	 * UpdateActions - 매 프레임 업데이트
	 * 
	 * 애니메이션 타이머를 업데이트하고, 애니메이션이 끝나면 다음 상태로 전환합니다.
	 * 게임 루프에서 매 프레임마다 호출해야 합니다.
	 * 
	 * @param dt 델타 타임 (초 단위)
	 */
	void UpdateActions(float dt)
	{
		// 이미 죽었으면 아무것도 안 함
		if (dyingDone_) return;

		// 프레임 드랍이 심할 때 시간이 너무 많이 흐르는 것 방지
		float cappedDt = std::min(dt, 0.1f);

		// 사망 애니메이션 처리
		if (dying_.playing)
		{
			dying_.timer += cappedDt;
			if (dying_.timer >= dying_.duration)
			{
				dying_.playing = false;
				dyingDone_ = true;  // 마지막 포즈에서 멈춤
			}
			else
			{
				Update(dt);
			}
			return;
		}

		// 애니메이션 업데이트
		Update(dt);

		// 공격 애니메이션 처리
		if (attack_.playing)
		{
			attack_.timer += cappedDt;
			if (attack_.timer >= attack_.duration)
			{
				// 공격 끝나면 idle로 복귀
				PlayAnimation("idle");
				LogAnimChange("Enemy", "idle", animState_);
				attack_.playing = false;
			}
		}

		// 스턴 처리: 끝나면 펀치로 전환
		if (stun_.playing)
		{
			stun_.timer += cappedDt;
			if (stun_.timer >= stun_.duration)
			{
				stun_.playing = false;
				StartPunch();  // 반격!
			}
		}

		// 펀치 처리: 끝나면 idle로 복귀
		if (punch_.playing)
		{
			punch_.timer += cappedDt;
			if (punch_.timer >= punch_.duration)
			{
				PlayAnimation("idle");
				LogAnimChange("Enemy", "idle", animState_);
				punch_.playing = false;
			}
		}
	}

	/*
	 * StartAttack - 적의 공격 시작
	 * 
	 * 적이 플레이어를 공격합니다.
	 * 
	 * @param target 공격 대상 (플레이어)
	 * @return 성공하면 true
	 */
	bool StartAttack(CharacterStatus& target)
	{
		if (!IsAlive() || attack_.playing) return false;
		if (!PlayAnimation("attack")) return false;

		LogAnimChange("Enemy", "attack", animState_);
		LogAnimStart("Enemy", "attack", attack_.duration);
		ApplyDamage(target, attack_.damage, "Enemy attack");

		attack_.playing = true;
		attack_.timer = 0.0f;
		return true;
	}

	/*
	 * TriggerStunSequence - 스턴 상태 시작
	 * 
	 * 플레이어의 공격을 받으면 기절합니다.
	 * 기절이 끝나면 자동으로 반격(펀치)합니다.
	 */
	void TriggerStunSequence()
	{
		if (!IsAlive()) return;
		if (!PlayAnimation("stun")) return;

		LogAnimChange("Enemy", "stun", animState_);
		LogAnimStart("Enemy", "stun", stun_.duration);

		// 다른 동작 중단
		attack_.playing = false;
		stun_.playing = true;
		stun_.timer = 0.0f;
		punch_.playing = false;
		punch_.timer = 0.0f;
	}

	/*
	 * StartDying - 사망 애니메이션 시작
	 * 
	 * HP가 0이 되면 호출됩니다.
	 */
	void StartDying()
	{
		if (dying_.playing || dyingDone_) return;
		if (!PlayAnimation("dying")) return;

		LogAnimChange("Enemy", "dying", animState_);
		LogAnimStart("Enemy", "dying", dying_.duration);

		dying_.playing = true;
		dying_.timer = 0.0f;
		dyingDone_ = false;

		// 다른 동작 중단
		attack_.playing = false;
		stun_.playing = false;
		punch_.playing = false;
	}

	// 상태 확인 함수들
	bool IsAlive() const { return status_.hp > 0 && !dying_.playing && !dyingDone_; }
	bool IsDying() const { return dying_.playing; }
	bool IsDead() const { return dyingDone_; }

	/*
	 * TakeDamage - 데미지 받기
	 * 
	 * HP가 깎이고, 0이 되면 자동으로 사망 애니메이션이 재생됩니다.
	 * 
	 * @param amount 데미지 양
	 * @param source 데미지 원인
	 */
	void TakeDamage(int amount, const char* source)
	{
		status_.hp = std::max(0, status_.hp - amount);
		std::cout << "[HP] " << status_.name << " -" << amount << " (from " << source << ") -> " << status_.hp << std::endl;

		// HP가 0이 되면 사망
		if (status_.hp == 0 && !dying_.playing && !dyingDone_)
		{
			StartDying();
		}
	}

	// 펀치로 플레이어에게 데미지를 주기 위한 타겟 설정
	void SetPunchTarget(CharacterStatus* target) { punchTarget_ = target; }

private:
	/*
	 * StartPunch - 펀치(반격) 시작
	 * 
	 * 스턴이 끝나면 자동으로 호출됩니다.
	 */
	void StartPunch()
	{
		if (!IsAlive()) return;
		if (!PlayAnimation("punch")) return;

		LogAnimChange("Enemy", "punch", animState_);
		LogAnimStart("Enemy", "punch", punch_.duration);

		// 플레이어에게 데미지
		if (punchTarget_)
		{
			ApplyDamage(*punchTarget_, punch_.damage, "Enemy punch");
		}

		punch_.playing = true;
		punch_.timer = 0.0f;
	}

	/*
	 * Action - 액션 정보 구조체
	 * 
	 * 각 동작(공격, 스턴 등)의 정보를 담습니다.
	 */
	struct Action
	{
		std::string key;       // 애니메이션 키
		int damage = 10;       // 데미지 양
		float duration = 1.0f; // 지속 시간
		float timer = 0.0f;    // 현재 경과 시간
		bool playing = false;  // 재생 중인지
	};

	// 각 액션 정의
	Action attack_{ "attack", 10 };
	Action stun_{ "stun", 0 };      // 스턴은 데미지 없음
	Action punch_{ "punch", 10 };
	Action dying_{ "dying", 0 };    // 사망도 데미지 없음

	bool dyingDone_ = false;                   // 사망 애니메이션 완료 여부
	std::string animState_ = "idle";           // 현재 애니메이션 상태 (로그용)
	CharacterStatus* punchTarget_ = nullptr;   // 펀치 대상 (플레이어)
};

#endif // ENEMY_H
