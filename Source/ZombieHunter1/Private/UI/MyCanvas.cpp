// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/MyCanvas.h"
#include "Characters/MyPlayer.h"
#include "Kismet/GameplayStatics.h" //getCharacter, sound
#include "UI/VirtualJoystick.h"
#include "Engine/Engine.h" //GEngine 화면 디버그
#include "UObject/Stack.h"  //[디버그] FFrame::GetScriptCallstack
#include "TimerManager.h"   //[디버그] 버튼 가시성 감시 타이머
#include "Engine/World.h"

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

    // [디버그] 재시작 버튼 가시성 추적 — 원인 잡히면 이 블록은 삭제할 것
    // SetVisRestartButton을 안 거치고 BP가 버튼 SetVisibility를 직접 부르는 경우까지 잡기 위해
    // 0.2초 간격으로 가시성 변화를 감시한다. (변화 시각과 함께 로그)
    if (RestartButton && GetWorld())
    {
        UE_LOG(LogTemp, Warning, TEXT("[RestartButton] NativeConstruct 시점 초기 가시성 = %s"),
            RestartButton->IsVisible() ? TEXT("보임") : TEXT("숨김"));

        FTimerHandle WatchHandle;
        TWeakObjectPtr<UButton> WatchedButton = RestartButton;
        TSharedRef<bool> LastVisible = MakeShared<bool>(RestartButton->IsVisible());
        GetWorld()->GetTimerManager().SetTimer(WatchHandle,
            FTimerDelegate::CreateWeakLambda(this, [this, WatchedButton, LastVisible]()
            {
                if (!WatchedButton.IsValid()) { return; }
                const bool bNow = WatchedButton->IsVisible();
                if (bNow != *LastVisible)
                {
                    UE_LOG(LogTemp, Warning, TEXT("[RestartButton] 가시성 변경 감지: %s @ %.2fs"),
                        bNow ? TEXT("보임") : TEXT("숨김"),
                        GetWorld() ? GetWorld()->GetTimeSeconds() : -1.f);
                    *LastVisible = bNow;
                }
            }),
            0.2f, true);
    }

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

void UMyCanvas::UpdateExp(int32 Level, int32 Exp, int32 ExpToNext)
{
    // 위젯은 옵셔널(BindWidgetOptional) — BP_Canvas에 아직 안 배치했으면 조용히 넘어간다.
    if (ExpText)
    {
        ExpText->SetText(FText::FromString(
            FString::Printf(TEXT("Lv.%d  %d / %d"), Level, Exp, ExpToNext)));
    }
    if (ExpBar)
    {
        ExpBar->SetPercent(ExpToNext > 0 ? (float)Exp / (float)ExpToNext : 0.0f);
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

    // [디버그] 누가 버튼을 켜는지 추적 — 원인 잡히면 이 블록은 삭제할 것
    {
        const float Time = GetWorld() ? GetWorld()->GetTimeSeconds() : -1.f;
        UE_LOG(LogTemp, Warning, TEXT("[RestartButton] SetVisRestartButton(%s) @ %.2fs"),
            isVis ? TEXT("true(켜기)") : TEXT("false(끄기)"), Time);
        if (isVis)
        {
            // BP에서 호출됐다면 어느 BP 노드에서 왔는지 스크립트 스택이 찍힌다.
            // 비어 있으면 C++(OnDeath/SetCanvasWidget)에서 직접 호출된 것.
            const FString ScriptStack = FFrame::GetScriptCallstack();
            UE_LOG(LogTemp, Warning, TEXT("[RestartButton] 켜기 호출자 스크립트 스택: %s"),
                ScriptStack.IsEmpty() ? TEXT("(없음 = C++에서 호출)") : *ScriptStack);
        }
    }

    if (RestartButton)
    {
        ESlateVisibility ev = isVis ? ESlateVisibility::Visible : ESlateVisibility::Hidden;
        RestartButton->SetVisibility(ev);
    }
}
