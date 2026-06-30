// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/Companion.h"
#include "Jobs/JobComponent.h"
#include "Characters/Enemy.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "DrawDebugHelpers.h" //진단 디버그
#include "Engine/Engine.h"     //GEngine 화면 메시지

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

	SetHP(MaxHP); // 동료도 체력을 가진다(베이스). 적에게 맞으면 닳고 0이면 죽는다.

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
			// 공격 몽타주는 이 동료(JobAttackMontages[JobName])가 자기 스켈레톤에 맞게 소유한다.
		}
	}

	// 공격 몽타주 Notify 배선은 베이스(ACombatCharacter)가 처리한다(→ HandleAttackNotify).
}

void ACompanion::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsDead)
	{
		return; // 죽었으면 추격/공격 로직 정지
	}

	TimeSinceAttack += DeltaTime;

	AEnemy* Target = FindNearestEnemy();

	// 진단: 직업 유무 + 가장 가까운 적까지 거리. 화면 좌상단에 매 프레임 갱신.
	//  job=NULL → DefaultJobClass 미설정 / target=NONE → 탐지반경(DetectRadius) 안에 적 없음.
	if (bDebugCombat && GEngine)
	{
		const FString Msg = FString::Printf(TEXT("[Companion] job=%s  target=%s"),
			CurrentJob ? *CurrentJob->JobName.ToString() : TEXT("NULL"),
			Target ? *FString::Printf(TEXT("dist %.0f (engage %.0f)"),
				FVector::Dist(GetActorLocation(), Target->GetActorLocation()),
				(CurrentJob && CurrentJob->EngageRange > 0.0f) ? CurrentJob->EngageRange : AttackRange) : TEXT("NONE"));
		GEngine->AddOnScreenDebugMessage((int32)GetUniqueID(), 0.0f, FColor::Cyan, Msg);
	}

	if (Target)
	{
		// === 교전 ===
		State = EState::Fighting;

		// 멈춰서 공격하는 거리 = 직업의 교전 사거리(근접=짧게, 원거리=길게). 직업 없으면 동료 AttackRange로 폴백.
		// 이게 직업의 실제 사거리와 따로 놀면, 사거리 밖에서 멈춰 영영 공격을 안 하는 버그가 생긴다.
		const float Engage = (CurrentJob && CurrentJob->EngageRange > 0.0f) ? CurrentJob->EngageRange : AttackRange;

		const float Dist = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
		if (Dist > Engage)
		{
			// 사거리 밖 → 적에게 접근 (조금 더 가깝게 멈추도록 수용 반경을 줄임)
			if (AICon)
			{
				AICon->MoveToActor(Target, Engage * 0.8f);
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

				if (bDebugCombat) // 공격을 실제로 호출하는 순간 표시(노란 선 + 메시지)
				{
					DrawDebugLine(GetWorld(), GetActorLocation(),
						Target->GetActorLocation(), FColor::Yellow, false, 0.4f, 0, 3.0f);
					if (GEngine)
					{
						GEngine->AddOnScreenDebugMessage(-1, 0.4f, FColor::Yellow,
							TEXT("[Companion] Attack() 호출!"));
					}
				}
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

void ACompanion::HandleAttackNotify(FName NotifyName)
{
	if (CurrentJob)
	{
		CurrentJob->OnAttackNotify(NotifyName);
	}
}

void ACompanion::OnDeath()
{
	// 추격/공격을 끊고, 시체가 길을 막지 않게 콜리전·이동을 끈 뒤 잠시 후 제거.
	if (AICon)
	{
		AICon->StopMovement();
		AICon->ClearFocus(EAIFocusPriority::Gameplay);
	}

	StopAnimMontage();

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	SetLifeSpan(CorpseLifeSpan); // 일정 시간 후 자동 Destroy (리더의 Companions 목록은 다음 섭외 때 정리됨)
}
