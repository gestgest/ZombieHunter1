// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "MyCanvas.generated.h"

class UVirtualJoystick;

UCLASS()
class ZOMBIEHUNTER1_API UMyCanvas : public UUserWidget
{
	GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;

public:

    //변수들
    // CoinText와 자동으로 바인딩됨
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* CoinText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UImage* hp_bar;

    UPROPERTY(meta = (BindWidget))
    UButton* RestartButton;

    // BP_Canvas에 배치한 조이스틱 인스턴스. 이름이 MoveJoystick / AimJoystick 이어야 자동 연결됨
    UPROPERTY(meta = (BindWidgetOptional))
    UVirtualJoystick* MoveJoystick;

    UPROPERTY(meta = (BindWidgetOptional))
    UVirtualJoystick* AimJoystick;

    // 경험치 표시 (선택) — BP_Canvas에 이 이름으로 배치하면 자동 연결, 없어도 컴파일에 지장 없음.
    // ExpText: "Lv.3  12 / 20" 형식 텍스트, ExpBar: 다음 레벨까지 진행도(0~1)
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* ExpText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UProgressBar* ExpBar;


    //BlueprintCallable 함수들
    UFUNCTION(BlueprintCallable)
    void RestartGame();

    
    void UpdateCoinText(int32 Money);
    void SetProgressUISize(FVector2D size);
    void SetVisRestartButton(bool isVis);

    /** 경험치 HUD 갱신 — AMyPlayer::UpdateExpUI가 호출. 위젯이 배치돼 있을 때만 그린다. */
    void UpdateExp(int32 Level, int32 Exp, int32 ExpToNext);

};
