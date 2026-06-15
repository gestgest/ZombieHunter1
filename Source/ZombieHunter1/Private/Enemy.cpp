// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy.h"
#include "MyPlayer.h"
#include "NavigationSystem.h"  // 내비게이션 사용 시
#include "Kismet/GameplayStatics.h" //getCharacter, sound
//#include "Kismet/KismetSystemLibrary.h" //ray


#include "Navigation/PathFollowingComponent.h" //GetPathFollowingComponent에 있는 OnRequestFinished
#include "AITypes.h"


// Sets default values
AEnemy::AEnemy()
{
	PrimaryActorTick.bCanEverTick = true; //update

	// 기본 AI 컨트롤러 클래스 지정 — SpawnDefaultController()가 항상 생성할 대상을 갖도록.
	// (BP에서 None으로 비워두면 SpawnDefaultController가 아무것도 안 만들어 크래시 위험)
	AIControllerClass = AAIController::StaticClass();
}

// Called when the game starts or when spawned
void AEnemy::BeginPlay()
{
	Super::BeginPlay();
	SetHP(5);
	CanAttack = true;
	Damage = 1;
	
	UAnimInstance* animInstance = Cast<UAnimInstance>(GetMesh()->GetAnimInstance());
	aiController = Cast<AAIController>(GetController());

	if (animInstance)
	{
		//AttackMontage-> 몽타주 세팅
		animInstance->OnMontageEnded.AddDynamic(this, &AEnemy::OnMontageEnded);

		animInstance->OnPlayMontageNotifyBegin.AddDynamic(
			this, &AEnemy::OnNotifyBeginReceived
		); //신호
	}

	if (aiController)
	{
		//추격을 완료했다면 - 콜백
		aiController->GetPathFollowingComponent()->OnRequestFinished.AddUObject(this, &AEnemy::MoveCompleted);
	}
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
	AMyPlayer* myPlayer = Cast<AMyPlayer>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));

	//isDead라면 추격하면 안됨
	if (myPlayer->checkDead() || IsDead)
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

	if (Result.Code == EPathFollowingResult::Success)
	{
		if (CanAttack)
		{
			PlayAnimMontage(AttackMontage); //지속시간 매개변수 있음
			//UE_LOG(LogTemp, Warning, TEXT("Montage Duration: %f"), duration);
			CanAttack = false;
		}
	}

}

//공격 몽타주 애니메이션이 끝났다면
void AEnemy::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	CanAttack = true;
}

//모든 노디파이 관리
void AEnemy::OnNotifyBeginReceived(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointPayload)
{
	//UE_LOG(LogTemp, Log, TEXT("attack signal"));
	if (NotifyName == FName("Attack"))
	{
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
		else
		{
			//GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::White, FString::Printf(TEXT("NO")));
		}
	}
}


//일단 때리는 애니메이션
bool AEnemy::hit()
{
	bool isAttack = false;

	//여기에 함수 죄다 넣어야 함
	TArray<FHitResult> hitResults;
	FVector start = GetActorLocation();
	FVector end = start + (GetActorForwardVector() * attackRange);
	

	FCollisionShape sphere = FCollisionShape::MakeSphere(attackRange);
	FCollisionQueryParams queryParams;
	queryParams.AddIgnoredActor(this);

	bool bHit = GetWorld()->SweepMultiByChannel(
		hitResults,
		start,
		end,
		FQuat::Identity,
		ECC_Pawn, //TraceChannel
		sphere,
		queryParams
	);
	// 끝 위치에 구 그리기
	//DrawDebugSphere(
	//	GetWorld(),
	//	end,
	//	attackRange,
	//	32,
	//	FColor::Green,
	//	false,
	//	2.0f
	//);

	// Sweep 경로를 선으로 연결
	DrawDebugLine(
		GetWorld(),
		start,
		end,
		FColor::Red,
		false,
		2.0f,
		0,
		2.0f
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

	//death
	SetIsDead(this->HP <= 0);
}

void AEnemy::SetID(int id)
{
	this->enemy_id = id;
}

void AEnemy::SetIsDead(bool value)
{
	IsDead = value;

	if (value)
	{
		DeadEnemySignal(enemy_id);
	}
}

void AEnemy::SetAIController()
{
	// 컨트롤러가 없을 때만 생성 (이미 빙의돼 있으면 중복 생성 방지)
	if (!GetController())
	{
		SpawnDefaultController(); //강제 호출
	}

	aiController = Cast<AAIController>(GetController());

	// SpawnDefaultController가 실패하면(AIControllerClass 미지정 등) aiController가 null일 수 있음.
	// null인 채로 역참조하면 크래시 → 방어적으로 검사한다. => 가끔 에디터가 터질때 이게 터진거다.
	if (!aiController)
	{
		UE_LOG(LogTemp, Warning, TEXT("AEnemy::SetAIController - AIController 생성 실패 (AIControllerClass 확인 필요) "));
		return;
	}

	if (UPathFollowingComponent* PFC = aiController->GetPathFollowingComponent())
	{
		PFC->OnRequestFinished.AddUObject(this, &AEnemy::MoveCompleted);
	}
}
