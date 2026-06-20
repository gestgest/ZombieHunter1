// Fill out your copyright notice in the Description page of Project Settings.

#include "Jobs/HealerJob.h"
#include "Characters/CombatCharacter.h"
#include "Characters/Enemy.h"
#include "Engine/World.h"

UHealerJob::UHealerJob()
{
	JobName = TEXT("Healer");
	// 힐러는 적을 때리지 않는다. "공격"은 힐 시전이고, 닿은 아군을 회복한다.
}

void UHealerJob::Attack()
{
	// 힐 시전 모션(몽타주)이 있으면 재생. 힐 자체는 아래에서 즉시 처리한다.
	Super::Attack();

	if (!OwnerCharacter)
	{
		return;
	}

	// 1) 시전자 자신 회복 — HP는 이제 공용 베이스(ACombatCharacter)에 있어 플레이어/동료 모두 가능.
	if (ACombatCharacter* Self = Cast<ACombatCharacter>(OwnerCharacter))
	{
		HealCharacter(Self);
	}

	// 2) 전방 범위의 아군(동료)도 회복 — 동료 시스템용
	UWorld* World = OwnerCharacter->GetWorld();
	if (!World)
	{
		return;
	}

	TArray<FHitResult> Hits;
	const FVector Start = OwnerCharacter->GetActorLocation();
	const FVector End = Start + OwnerCharacter->GetActorForwardVector() * HealRange;

	FCollisionShape Sphere = FCollisionShape::MakeSphere(HealRadius);
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerCharacter); // 자신은 위에서 이미 회복

	World->SweepMultiByChannel(Hits, Start, End, FQuat::Identity, ECC_Pawn, Sphere, Params);

	for (const FHitResult& Hit : Hits)
	{
		// 아군(플레이어/동료)만 회복한다 — 적(AEnemy)은 제외.
		ACombatCharacter* Ally = Cast<ACombatCharacter>(Hit.GetActor());
		if (Ally && !Ally->IsA(AEnemy::StaticClass()))
		{
			HealCharacter(Ally);
		}
	}
}

void UHealerJob::HealCharacter(ACombatCharacter* Target)
{
	if (!Target)
	{
		return;
	}

	// 최대 체력 미만일 때만 회복. SetHP가 HUD 체력바도 갱신한다.
	if (Target->HP < MaxHP)
	{
		const int32 NewHP = FMath::Min(Target->HP + HealAmount, MaxHP);
		Target->SetHP(NewHP);
	}
}
