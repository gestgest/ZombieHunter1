// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy.h"
#include "MyPlayer.h"
#include "AIController.h"
#include "NavigationSystem.h"  // 내비게이션 사용 시
#include "Kismet/GameplayStatics.h" //getCharacter, sound
//#include "Kismet/KismetSystemLibrary.h" //ray


#include "Navigation/PathFollowingComponent.h" //GetPathFollowingComponent에 있는 OnRequestFinished
#include "AITypes.h"


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
	CanAttack = true;
	Damage = 1;
	SetHP(5);
	
	UAnimInstance* animInstance = Cast<UAnimInstance>(GetMesh()->GetAnimInstance());
	aiController = Cast<AAIController>(GetController());

	if (animInstance)
	{
		//AttackMontage-> 몽타주 세팅
		animInstance->OnMontageEnded.AddDynamic(this, &AEnemy::OnAttackMontageEnded);
		animInstance->OnPlayMontageNotifyBegin.AddDynamic(
			this, &AEnemy::OnNotifyBeginReceived
		); //신호
	}

	//추격을 완료했다면 - 콜백
	if (aiController)
	{
		aiController->GetPathFollowingComponent()->OnRequestFinished.AddUObject(this, &AEnemy::MoveCompleted);
	}
}

// Called every frame
void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	DebugHPShow();
}

// Called to bind functionality to input
void AEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

//추격하는 함수
void AEnemy::TrackingPlayer()
{
	AMyPlayer* myPlayer = Cast<AMyPlayer>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));

	//isDead라면 추격하면 안됨
	if (myPlayer->checkDead())
	{
		return;
	}

	if (aiController)
	{
		aiController->SetFocus(myPlayer);
		aiController->MoveToActor(myPlayer, attackRange);
	}
}

//추격을 완료했다면
void AEnemy::MoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	//USkeletalMeshComponent* mesh = GetMesh();
	if (CanAttack)
	{
		PlayAnimMontage(AttackMontage); //지속시간 매개변수 있음
		//UE_LOG(LogTemp, Warning, TEXT("Montage Duration: %f"), duration);
		CanAttack = false;
	}
}

//공격 몽타주 애니메이션이 끝났다면
void AEnemy::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	//UE_LOG(LogTemp, Log, TEXT("OnAttackMontageEnded"));
	CanAttack = true;
}

void AEnemy::OnNotifyBeginReceived(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointPayload)
{
	//UE_LOG(LogTemp, Log, TEXT("attack signal"));

	if (hit())
	{
		//사운드
		if (AttackSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,  // WorldContextObject
				AttackSound,
				GetActorLocation(),
				1.0f,  // VolumeMultiplier
				1.0f   // PitchMultiplier
			);
		}
	}
}

bool AEnemy::hit()
{
	bool isAttack = false;

	//여기에 함수 죄다 넣어야 함
	TArray<FHitResult> hitResults;
	FVector start = GetActorLocation();
	FVector end = start + (GetActorForwardVector() * 150.0f);
	

	FCollisionShape Sphere = FCollisionShape::MakeSphere(attackRange);
	FCollisionQueryParams queryParams;
	queryParams.AddIgnoredActor(this);

	//SweepSingleByChanne
	bool bHit = GetWorld()->SweepMultiByChannel(
		hitResults,
		start,
		end,
		FQuat::Identity,
		ECC_Pawn, //TraceChannel
		Sphere,
		queryParams
	);

	for (const FHitResult& hit : hitResults)
	{
		AMyPlayer* hitCharacter = Cast<AMyPlayer>(hit.GetActor());
		if (hitCharacter && hitCharacter->IsPlayerControlled())
		{
			//UE_LOG(LogTemp, Log, TEXT("Hit Player!"));
			hitCharacter->AddHP(-Damage);
			FVector force = GetActorForwardVector() * 500 + FVector(0, 0, 100);
			hitCharacter->LaunchCharacter(force, false, false);
			isAttack = true;
		}
	}
	return isAttack;
}

void AEnemy::DebugHPShow()
{
	UE_LOG(LogTemp, Log, TEXT("bobo : %d"), HP);
}

void AEnemy::AddHP(int add_hp)
{
	SetHP(this->HP + add_hp);
}

void AEnemy::SetHP(int new_hp)
{
	this->HP = new_hp;

	if (this->HP <= 0)
	{
		IsDead = true;
	}
	else {
		IsDead = false;
	}
}
