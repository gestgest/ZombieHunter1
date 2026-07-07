// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "ZombieGameInstance.generated.h"

class UJobComponent;

/**
 * POI 하나의 저장 상태 (v1: 발판).
 * 시드로 재계산 가능한 것(위치/타입/바닥)은 저장하지 않는다 —
 * 플레이어가 바꿔서 시드로는 복구 불가능한 "차이(delta)"만 담는다.
 */
USTRUCT()
struct FPOIState
{
	GENERATED_BODY()

	/** 발판에 누적 결제한 돈 (게이지 진행도) */
	UPROPERTY()
	int32 PaidMoney = 0;

	/** 발판 완성 비용 — AWeaponUpgradeZone은 강화마다 CostGrowth로 오르므로 같이 지켜야 비용 리셋 악용이 없다 */
	UPROPERTY()
	int32 MaxMoney = 5;

	/** 일회성(bOneShot) 발판이 이미 쓰였는지 */
	UPROPERTY()
	bool bConsumed = false;
};

/**
 * 레벨(맵) 전환에도 파괴되지 않고 살아남는 유일한 객체 — 유니티의 DontDestroyOnLoad 역할.
 * 게임 전체에 딱 1개만 존재하므로, 레벨을 넘나들며 살아야 하는 데이터는 전부 여기 모인다:
 *  1) 직업 선택 씬에서 고른 직업을 게임플레이 맵까지 실어 나른다.
 *  2) 무한맵 POI(마을 발판)의 상태 — 청크 언로드/재생성에도 게이지가 이어지게 한다.
 *
 * 사용법:
 *  - Project Settings → Maps & Modes → Game Instance Class 를 이 클래스로 지정해야 작동한다.
 *  - 직업 선택 UI(BP)에서: Get Game Instance → Cast To ZombieGameInstance → Set Selected Job Class → Open Level
 *  - 플레이어(AMyPlayer::SetJob)가 스폰 시 이 값을 읽어 직업을 부착한다.
 *    None(선택 안 함)이면 기존처럼 플레이어의 DefaultJobClass를 쓴다 — 에디터 셋업 전에도 게임은 그대로 돈다.
 */
UCLASS()
class ZOMBIEHUNTER1_API UZombieGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	/** 직업 선택 씬에서 고른 직업 클래스. None이면 선택 안 한 것(플레이어 기본 직업 사용). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job")
	TSubclassOf<UJobComponent> SelectedJobClass;

	/** POI 중심 청크 좌표 → 상태. 청크 언로드/레벨 종료 시 저장되고, 청크 재생성 시 복원된다.
	 *  기본값과 같은 상태는 저장하지 않으므로 "플레이어가 손댄 POI"만큼만 자란다. */
	UPROPERTY()
	TMap<FIntPoint, FPOIState> POIStates;
};
