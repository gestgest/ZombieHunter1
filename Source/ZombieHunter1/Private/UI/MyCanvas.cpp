// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/MyCanvas.h"
#include "Characters/MyPlayer.h"
#include "Kismet/GameplayStatics.h" //getCharacter, sound
#include "UI/VirtualJoystick.h"
#include "Engine/Engine.h" //GEngine 화면 디버그

void UMyCanvas::NativeConstruct()
{
    Super::NativeConstruct();

    // 모바일(안드로이드/iOS)에서만 터치 조이스틱 표시, PC에선 숨김.
    // 위젯이 만들어질 때 자동 실행되므로 BP가 SetCanvasWidget을 호출하든 말든 항상 적용됨.
#if PLATFORM_ANDROID || PLATFORM_IOS
    const ESlateVisibility JoystickVis = ESlateVisibility::Visible;
#else
    const ESlateVisibility JoystickVis = ESlateVisibility::Collapsed;
#endif
    if (MoveJoystick) { MoveJoystick->SetVisibility(JoystickVis); }
    if (AimJoystick)  { AimJoystick->SetVisibility(JoystickVis); }

    // 포인터 바인딩 확인용: NULL이면 BP_Canvas의 위젯 이름이 MoveJoystick/AimJoystick과
    // 일치하지 않는다는 뜻 → 이 경우 조이스틱이 안 숨겨진다(BindWidgetOptional).
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Yellow,
            FString::Printf(TEXT("[Canvas] MoveJoystick=%s, AimJoystick=%s"),
                MoveJoystick ? TEXT("OK") : TEXT("NULL"),
                AimJoystick ? TEXT("OK") : TEXT("NULL")));
    }
}

void UMyCanvas::UpdateCoinText(int32 Money)
{
    if (CoinText)
    {
        FText MoneyText = FText::Format(
            FText::FromString(TEXT("{0} 원")),
            FText::AsNumber(Money)
        );
        CoinText->SetText(MoneyText);
    }
}

void UMyCanvas::SetProgressUISize(FVector2D size)
{
    if (hp_bar)
    {
        UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(hp_bar);
        if (CanvasSlot)
        {
            CanvasSlot->SetSize(size);
            //UE_LOG(LogTemp, Log, TEXT("hp_bar size set to: %s"), *size.ToString());
        }
    }
}

void UMyCanvas::RestartGame()
{
    AMyPlayer* myPlayer = Cast<AMyPlayer>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
    myPlayer->ReStart();
}

void UMyCanvas::SetVisRestartButton(bool isVis)
{
    //ESlateVisibility::Visible 보이는 거
    //ESlateVisibility::Hidden 안 보이는 거
    if (RestartButton)
    {
        ESlateVisibility ev = isVis ? ESlateVisibility::Visible : ESlateVisibility::Hidden;
        RestartButton->SetVisibility(ev);
    }
}
