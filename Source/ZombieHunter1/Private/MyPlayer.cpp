// Fill out your copyright notice in the Description page of Project Settings.


#include "MyPlayer.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"

// Sets default values
AMyPlayer::AMyPlayer()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AMyPlayer::BeginPlay()
{
	Super::BeginPlay();
    Money = 1;

}

// Called every frame
void AMyPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AMyPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AMyPlayer::AddMoney()
{
	//GetTargetLocation();
	Money++;

    // 위젯이 생성되어 있는지 확인
    if (CanvasWidget)
    {
        // 위젯에서 CoinText 찾기
        UTextBlock* CoinText = Cast<UTextBlock>(CanvasWidget->GetWidgetFromName(TEXT("CoinText")));

        if (CoinText)
        {
            // int를 FText로 변환하고 포맷팅
            FText MoneyText = FText::Format(
                FText::FromString(TEXT("{0} 원")),
                FText::AsNumber(Money)
            );

            // CoinText에 설정
            CoinText->SetText(MoneyText);
        }
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("PPAP"));
    }
}