// Fill out your copyright notice in the Description page of Project Settings.

#include "Companion.h"
#include "Jobs/JobComponent.h"
#include "Enemy.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"

ACompanion::ACompanion()
{
	PrimaryActorTick.bCanEverTick = true;

	// 따라다닐 땐 이동 방향으로 자연스럽게 회전. 공격 시엔 코드(FaceActor)로 적을 바라보게 덮어쓴다.
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);

	// 스폰되면 자동으로 AI 컨트롤러가 빙의하도록.
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void ACompanion::BeginPlay()
{
	Super::BeginPlay();

	AICon = Cast<AAIController>(GetController());
	if (!AICon)
	{
		SpawnDefaultController();
		AICon = Cast<AAIController>(GetController());
	}

	// 직업 컴포넌트 생성 — 플레이어와 동일한 방식(같은 UJobComponent 재활용).
	if (DefaultJobClass)
	{
		CurrentJob = NewObject<UJobComponent>(this, DefaultJobClass);
		if (CurrentJob)
		{
			CurrentJob->RegisterComponent();
			CurrentJob->InitializeForOwner(this); // 소유자 = 이 동료(ACharacter)
			if (!CurrentJob->AttackMontage)
			{
				CurrentJob->AttackMontage = AttackMontage; // 직업에 몽타주 없으면 동료 값으로 보충
			}
		}
	}

	// 공격 몽타주 Notify → 직업의 OnAttackNotify (검사 근접 타격 판정 등).
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		if (UAnimInstance* Anim = MeshComp->GetAnimInstance())
		{
			Anim->OnPlayMontageNotifyBegin.AddDynamic(this, &ACompanion::OnNotifyBeginReceived);
		}
	}
}

void ACompanion::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TimeSinceAttack += DeltaTime;

	AEnemy* Target = FindNearestEnemy();

	if (Target)
	{
		// === 교전 ===
		State = EState::Fighting;

		const float Dist = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
		if (Dist > AttackRange)
		{
			// 사거리 밖 → 적에게 접근 (조금 더 가깝게 멈추도록 수용 반경을 줄임)
			if (AICon)
			{
				AICon->MoveToActor(Target, AttackRange * 0.8f);
			}
		}
		else
		{
			// 사거리 안 → 멈추고 적을 바라보며 일정 간격으로 공격
			if (AICon)
			{
				AICon->StopMovement();
			}
			FaceActor(Target);

			const float Interval = (CurrentJob && CurrentJob->AttackInterval > 0.0f)
				? CurrentJob->AttackInterval : AttackInterval;
			if (CurrentJob && TimeSinceAttack >= Interval)
			{
				TimeSinceAttack = 0.0f;
				CurrentJob->Attack(); // 직업이 공격(검 스윕/화살/파이어볼 등)
			}
		}
	}
	else if (Leader)
	{
		// === 따라다님 ===
		State = EState::Following;

		const float Dist = FVector::Dist(GetActorLocation(), Leader->GetActorLocation());
		if (Dist > FollowDistance)
		{
			if (AICon)
			{
				AICon->MoveToActor(Leader, FollowDistance);
			}
		}
		else if (AICon)
		{
			AICon->StopMovement(); // 충분히 가까우면 멈춤(졸졸 따라가다 정지)
		}
	}
}

AEnemy* ACompanion::FindNearestEnemy() const
{
	// v1: 단순히 모든 적을 훑어 가장 가까운 살아있는 적을 고른다.
	// (적이 많아지면 오버랩 쿼리로 최적화 가능)
	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemy::StaticClass(), Enemies);

	AEnemy* Best = nullptr;
	float BestDistSq = DetectRadius * DetectRadius;
	const FVector Loc = GetActorLocation();

	for (AActor* A : Enemies)
	{
		AEnemy* E = Cast<AEnemy>(A);
		if (!E || E->IsDead)
		{
			continue; // 죽은 적은 무시
		}
		const float DSq = FVector::DistSquared(Loc, E->GetActorLocation());
		if (DSq < BestDistSq)
		{
			BestDistSq = DSq;
			Best = E;
		}
	}
	return Best;
}

void ACompanion::FaceActor(AActor* Target)
{
	if (!Target)
	{
		return;
	}
	FVector Dir = Target->GetActorLocation() - GetActorLocation();
	Dir.Z = 0.0f;
	if (Dir.Normalize())
	{
		SetActorRotation(FRotator(0.0f, Dir.Rotation().Yaw, 0.0f));
	}
}

void ACompanion::OnNotifyBeginReceived(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointPayload)
{
	if (CurrentJob)
	{
		CurrentJob->OnAttackNotify(NotifyName);
	}
}
