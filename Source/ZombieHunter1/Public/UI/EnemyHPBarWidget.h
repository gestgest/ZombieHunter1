// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EnemyHPBarWidget.generated.h"

class UProgressBar;

/**
 * 적 머리 위 HP 바 — AEnemy의 스크린 스페이스 WidgetComponent에 꽂히는 위젯.
 * BP(WBP_EnemyHPBar)는 이 클래스를 부모로 두고 "HPBar"라는 이름의 ProgressBar만 배치하면 된다.
 */
UCLASS()
class ZOMBIEHUNTER1_API UEnemyHPBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// BP의 ProgressBar 이름이 정확히 "HPBar"여야 자동 바인딩된다 (다르면 컴파일 에러로 잡아줌)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UProgressBar* HPBar;

	/** 0~1로 채움 비율 설정. AEnemy::SetHP가 HP/MaxHP를 넘겨준다. */
	void SetHPPercent(float Percent);
};
