#pragma once

#include "CoreMinimal.h"
#include "MoneyPadZone.h"
#include "CompanionSpawnZone.generated.h"

/**
 * 동료 소환 발판 — 돈 발판(AMoneyPadZone) 공통 로직 위에 "완성 시 동료 소환"만 얹는다.
 * 트리거/결제/게이지/쿨다운/디버그 바는 전부 베이스가 처리한다.
 */
UCLASS()
class ZOMBIEHUNTER1_API ACompanionSpawnZone : public AMoneyPadZone
{
	GENERATED_BODY()

protected:
	// 게이지 완성 → 동료 소환 (C키 섭외와 동일 로직 재사용)
	virtual void HandleZoneFilled(AMyPlayer* Payer) override;
};
