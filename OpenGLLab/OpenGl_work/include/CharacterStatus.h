/*
 * CharacterStatus.h
 * 
 * 캐릭터의 기본 상태(이름, HP)를 저장하는 구조체와
 * 게임에서 자주 사용하는 유틸리티 함수들을 정의합니다.
 */

#ifndef CHARACTER_STATUS_H
#define CHARACTER_STATUS_H

#include <string>
#include <iostream>
#include <algorithm>

/*
 * CharacterStatus - 캐릭터 상태 구조체
 * 
 * 플레이어나 적 같은 캐릭터의 기본 정보를 담는 구조체입니다.
 * - name: 캐릭터 이름 (예: "Player", "Enemy")
 * - hp: 체력 (Health Point), 0이 되면 사망
 */
struct CharacterStatus
{
	std::string name;  // 캐릭터 이름
	int hp = 100;      // 체력 (기본값 100)
};

/*
 * LogAnimStart - 애니메이션 시작 로그 출력
 * 
 * 애니메이션이 시작될 때 콘솔에 정보를 출력합니다.
 * 디버깅할 때 어떤 애니메이션이 언제 시작됐는지 확인하기 좋습니다.
 * 
 * @param who         누구의 애니메이션인지 (예: "Player", "Enemy")
 * @param state       어떤 상태인지 (예: "attack", "idle")
 * @param durationSec 애니메이션 지속 시간 (초 단위)
 */
inline void LogAnimStart(const char* who, const char* state, double durationSec)
{
	std::cout << "[Anim] " << who << " " << state << " start, remaining=" << durationSec << " sec" << std::endl;
}

/*
 * LogAnimChange - 애니메이션 상태 변경 로그 출력
 * 
 * 애니메이션 상태가 바뀔 때만 로그를 출력합니다.
 * 같은 상태면 출력하지 않아서 콘솔이 지저분해지는 것을 방지합니다.
 * 
 * @param who   누구의 애니메이션인지
 * @param state 새로운 상태
 * @param prev  이전 상태 (참조로 받아서 업데이트함)
 */
inline void LogAnimChange(const char* who, const char* state, std::string& prev)
{
	if (prev != state)
	{
		prev = state;
		std::cout << "[Anim] " << who << " -> " << state << std::endl;
	}
}

/*
 * ApplyDamage - 데미지 적용
 * 
 * 대상 캐릭터에게 데미지를 주고 HP를 깎습니다.
 * HP는 0 미만으로 내려가지 않습니다.
 * 
 * @param target 데미지를 받을 캐릭터
 * @param amount 데미지 양
 * @param source 데미지 원인 (예: "Player attack")
 */
inline void ApplyDamage(CharacterStatus& target, int amount, const char* source)
{
	// HP에서 데미지를 빼되, 0보다 작아지지 않게 함
	target.hp = std::max(0, target.hp - amount);
	std::cout << "[HP] " << target.name << " -" << amount << " (from " << source << ") -> " << target.hp << std::endl;
}

#endif // CHARACTER_STATUS_H
