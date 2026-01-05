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
    Money = 0;
    if (CanvasWidget)
    {
        CanvasWidget->UpdateCoinText(Money);
    }

    //if (CanvasWidgetClass)
    //{
    //    CanvasWidget = CreateWidget<UMyCanvas>(GetWorld(), CanvasWidgetClass);
    //    if (CanvasWidget)
    //    {
    //        CanvasWidget->AddToViewport();
    //    }
    //}
}

// Called every frame
void AMyPlayer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

}

void AMyPlayer::SetCanvasWidget(UMyCanvas* cw)
{
    CanvasWidget = cw;
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

    if (CanvasWidget)
    {
        CanvasWidget->UpdateCoinText(Money);
    }

}

void AMyPlayer::SetProgressUISize(FVector2D size)
{
    if (CanvasWidget)
    {
        CanvasWidget->SetProgressUISize(size);
    }
}
