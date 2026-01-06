// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy.h"
#include "AIController.h"
#include "NavigationSystem.h"  // 내비게이션 사용 시
#include "Kismet/GameplayStatics.h" //getCharacter

// Sets default values
AEnemy::AEnemy()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AEnemy::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

//추격하는 함수
void AEnemy::TrackingPlayer()
{
	AAIController* AIController = Cast<AAIController>(GetController());
	ACharacter* playerPawn = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);

	if (AIController)
	{
		AIController->SetFocus(playerPawn);
		AIController->MoveToActor(playerPawn, 100);
		//콜백 넣어야함
	}
}
