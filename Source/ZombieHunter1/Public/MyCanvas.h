// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Button.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "MyCanvas.generated.h"

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


    //BlueprintCallable 함수들
    UFUNCTION(BlueprintCallable)
    void RestartGame();

    
    void UpdateCoinText(int32 Money);
    void SetProgressUISize(FVector2D size);
    void SetVisRestartButton(bool isVis);

};
