// Fill out your copyright notice in the Description page of Project Settings.

#include "Jobs/HealerJob.h"
#include "MyPlayer.h"
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

	if (!OwnerPlayer)
	{
		return;
	}

	// 1) 시전자 자신 회복 (동료가 없는 솔로에서도 유효하도록)
	HealCharacter(OwnerPlayer);

	// 2) 전방 범위의 아군(동료)도 회복 — 동료 시스템용
	UWorld* World = OwnerPlayer->GetWorld();
	if (!World)
	{
		return;
	}

	TArray<FHitResult> Hits;
	const FVector Start = OwnerPlayer->GetActorLocation();
	const FVector End = Start + OwnerPlayer->GetActorForwardVector() * HealRange;

	FCollisionShape Sphere = FCollisionShape::MakeSphere(HealRadius);
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerPlayer); // 자신은 위에서 이미 회복

	World->SweepMultiByChannel(Hits, Start, End, FQuat::Identity, ECC_Pawn, Sphere, Params);

	for (const FHitResult& Hit : Hits)
	{
		// 적(AEnemy)은 무시하고, 아군(플레이어/동료)만 회복한다.
		if (AMyPlayer* Ally = Cast<AMyPlayer>(Hit.GetActor()))
		{
			HealCharacter(Ally);
		}
		// TODO(동료 시스템): 동료 클래스도 여기서 Cast해서 HealCharacter로 회복
	}
}

void UHealerJob::HealCharacter(AMyPlayer* Target)
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
