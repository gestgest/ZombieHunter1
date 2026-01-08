// Fill out your copyright notice in the Description page of Project Settings.


#include "MyCanvas.h"
#include "MyPlayer.h"
#include "Kismet/GameplayStatics.h" //getCharacter, sound

void UMyCanvas::NativeConstruct()
{
    Super::NativeConstruct();
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
