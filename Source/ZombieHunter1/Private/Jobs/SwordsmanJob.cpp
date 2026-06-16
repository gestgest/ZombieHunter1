// Fill out your copyright notice in the Description page of Project Settings.

#include "Jobs/SwordsmanJob.h"
#include "MyPlayer.h"
#include "Enemy.h"
#include "Engine/World.h"
#include "CollisionShape.h"

USwordsmanJob::USwordsmanJob()
{
	JobName = TEXT("Swordsman");

	// 무기(WeaponMesh)는 BP 서브클래스(예: BP_SwordsmanJob)에서 직접 지정한다.
}

void USwordsmanJob::OnAttackNotify(FName NotifyName)
{
	if (!OwnerPlayer)
	{
		return;
	}

	// 전방으로 구체를 스윕해 범위 내 모든 적을 타격한다.
	TArray<FHitResult> hitResults;
	const FVector start = OwnerPlayer->GetActorLocation();
	const FVector end = start + (OwnerPlayer->GetActorForwardVector() * AttackRange);

	FCollisionShape Sphere = FCollisionShape::MakeSphere(AttackRadius);
	FCollisionQueryParams queryParams;
	queryParams.AddIgnoredActor(OwnerPlayer);

	const bool bHit = OwnerPlayer->GetWorld()->SweepMultiByChannel(
		hitResults,
		start,
		end,
		FQuat::Identity,
		ECC_Pawn, // TraceChannel
		Sphere,
		queryParams
	);

	bool bDidHit = false;
	for (const FHitResult& Result : hitResults)
	{
		AEnemy* hitEnemy = Cast<AEnemy>(Result.GetActor());
		if (hitEnemy)
		{
			UE_LOG(LogTemp, Log, TEXT("Hit Enemy!"));
			hitEnemy->AddHP(-Damage);
			const FVector force = OwnerPlayer->GetActorForwardVector() * KnockbackForce + FVector(0, 0, 100);
			hitEnemy->LaunchCharacter(force, false, false);
			bDidHit = true;
		}
	}

	// 적중했을 때만 공격 사운드 재생 (기존 동작 유지)
	if (bDidHit)
	{
		PlayAttackSound();
	}
}
