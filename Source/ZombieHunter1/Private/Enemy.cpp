// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy.h"
#include "MyPlayer.h"
#include "NavigationSystem.h"  // 내비게이션 사용 시
#include "Kismet/GameplayStatics.h" //getCharacter, sound
//#include "Kismet/KismetSystemLibrary.h" //ray


#include "Navigation/PathFollowingComponent.h" //GetPathFollowingComponent에 있는 OnRequestFinished
#include "AITypes.h"
#include "GameFramework/CharacterMovementComponent.h" //죽을 때 이동 정지
#include "Components/CapsuleComponent.h"             //죽을 때 충돌 해제


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

		// 공격 Notify 배선은 베이스(ACombatCharacter)가 처리한다(→ HandleAttackNotify).
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
	if (IsDead)
	{
		return; // 죽은 적은 추격 완료해도 공격하지 않음
	}

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

//공격 몽타주 Notify 처리(베이스에서 호출) — "Attack" 신호에 자체 타격을 수행.
void AEnemy::HandleAttackNotify(FName NotifyName)
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
	if (IsDead)
	{
		return false; // 죽은 적은 데미지를 주지 않음 (몽타주 노티파이가 늦게 터져도 무효)
	}

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
		// 아군(플레이어/동료)만 때린다 — 적(AEnemy)끼리는 무시. 둘 다 ACombatCharacter라 공용으로 처리.
		ACombatCharacter* Victim = Cast<ACombatCharacter>(hit.GetActor());
		if (Victim && !Victim->IsDead && !Victim->IsA(AEnemy::StaticClass()))
		{
			//UE_LOG(LogTemp, Log, TEXT("Hit Ally!"));
			Victim->AddHP(-Damage);
			FVector force = GetActorForwardVector() * 500 + FVector(0, 0, 100);
			Victim->LaunchCharacter(force, false, false);
			isAttack = true;
		}
	}
	return isAttack;
}

void AEnemy::DebugHPShow()
{
	UE_LOG(LogTemp, Log, TEXT("bobo : %d"), HP);
}

void AEnemy::SetID(int id)
{
	this->enemy_id = id;
}

// 살아있음 → 죽음 전환 시 베이스가 1회 호출. (HP/IsDead 갱신은 베이스 담당)
void AEnemy::OnDeath()
{
	// 죽는 즉시 추격/공격/이동을 끊는다 — 죽으면서 쫓아오거나 때리는 버그 방지.
	CanAttack = false;

	if (aiController)
	{
		aiController->StopMovement();                       // 진행 중이던 추격 취소
		aiController->ClearFocus(EAIFocusPriority::Gameplay); // 플레이어 주시 해제
	}

	StopAnimMontage(); // 진행 중이던 공격 몽타주 중단 (공격 노티파이 추가 발동 방지)

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
	}

	// 시체가 플레이어를 밀거나 길을 막지 않도록 캡슐 충돌 해제
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	DeadEnemySignal(enemy_id); // BP: 사망 애님 재생 / 일정 시간 후 Destroy
}

// 죽음 → 부활 전환 시 베이스가 1회 호출 (게임모드 풀에서 SetHP(5)로 재사용).
// 이게 없으면 재활용된 적은 캡슐 콜리전이 꺼진 채라 공격 스윕(ECC_Pawn)에 안 맞고 피도 안 닳는다.
void AEnemy::OnRevive()
{
	CanAttack = true;

	// 캡슐 충돌 복구 — 이게 핵심. 안 켜면 때려도 SweepMultiByChannel에 안 잡힘.
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}

	// 이동 복구 — 죽을 때 DisableMovement()로 MOVE_None이 됐던 걸 다시 걷게.
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->SetMovementMode(MOVE_Walking);
	}
}

void AEnemy::SetAIController()
{
	// 이미 AI 컨트롤러에 빙의돼 있으면 그대로 재사용
	aiController = Cast<AAIController>(GetController());

	if (!aiController)
	{
		// 혹시 BP/직렬화로 AIControllerClass가 비어 있을 경우를 대비해 기본값을 런타임에 보장
		// (C++ 생성자 값보다 BP 저장값이 우선하므로, 여기서 한 번 더 못 박는다)
		if (AIControllerClass == nullptr)
		{
			AIControllerClass = AAIController::StaticClass();
		}

		// 비-AI 컨트롤러가 이미 빙의 중이면 SpawnDefaultController가 그냥 무시된다
		// (내부에서 Controller != null이면 early-return). 먼저 빙의를 풀어준다.
		if (AController* Existing = GetController())
		{
			Existing->UnPossess();
		}

		SpawnDefaultController(); //AI 컨트롤러 생성 + 빙의
		aiController = Cast<AAIController>(GetController());
	}

	// 그래도 실패하면 크래시 대신 경고. 정확한 원인 파악을 위해 상태를 모두 찍는다.
	if (!aiController)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("AEnemy::SetAIController 실패 - AIControllerClass=%s, Controller=%s, AutoPossessAI=%d"),
			*GetNameSafe(AIControllerClass),
			*GetNameSafe(GetController()),
			(int32)AutoPossessAI);
		return;
	}

	if (UPathFollowingComponent* PFC = aiController->GetPathFollowingComponent())
	{
		PFC->OnRequestFinished.AddUObject(this, &AEnemy::MoveCompleted);
	}
}
