// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "ZombieGameInstance.generated.h"

class UJobComponent;

/**
 * 레벨(맵) 전환에도 파괴되지 않고 살아남는 유일한 객체 — 유니티의 DontDestroyOnLoad 역할.
 * 직업 선택 씬에서 고른 직업을 게임플레이 맵까지 실어 나른다.
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
};
