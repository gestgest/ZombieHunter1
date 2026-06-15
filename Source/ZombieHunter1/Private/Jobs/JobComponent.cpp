// Fill out your copyright notice in the Description page of Project Settings.

#include "Jobs/JobComponent.h"
#include "MyPlayer.h"
#include "Projectiles/Projectile.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

UJobComponent::UJobComponent()
{
	// 직업 컴포넌트는 Tick이 필요 없다 (공격은 플레이어가 타이밍을 호출).
	PrimaryComponentTick.bCanEverTick = false;
}

void UJobComponent::InitializeForOwner(AMyPlayer* Player)
{
	OwnerPlayer = Player;
}

void UJobComponent::Attack()
{
	// 기본 동작: 공격 몽타주 재생. 몽타주의 Notify가 OnAttackNotify()를 부른다.
	if (OwnerPlayer && AttackMontage)
	{
		OwnerPlayer->PlayAnimMontage(AttackMontage);
	}
}

void UJobComponent::OnAttackNotify(FName NotifyName)
{
	// 기본 구현 없음. 직업별 서브클래스에서 재정의한다.
}

void UJobComponent::TickJob(float DeltaTime)
{
	// 기본 구현 없음. 패시브가 필요한 직업(힐러 등)에서 재정의한다.
}

void UJobComponent::PlayAttackSound()
{
	if (OwnerPlayer && AttackSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			OwnerPlayer,
			AttackSound,
			OwnerPlayer->GetActorLocation(),
			1.0f,
			1.0f
		);
	}
}

AProjectile* UJobComponent::SpawnProjectileForward(TSubclassOf<AProjectile> ProjectileClass, float Speed, float MuzzleOffset, float MuzzleHeight)
{
	if (!OwnerPlayer || !ProjectileClass)
	{
		return nullptr;
	}

	UWorld* World = OwnerPlayer->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const FVector Forward = OwnerPlayer->GetActorForwardVector();
	const FVector SpawnLocation =
		OwnerPlayer->GetActorLocation() + Forward * MuzzleOffset + FVector(0, 0, MuzzleHeight);
	const FRotator SpawnRotation = Forward.Rotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerPlayer;
	SpawnParams.Instigator = OwnerPlayer;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AProjectile* Projectile = World->SpawnActor<AProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (Projectile)
	{
		Projectile->Damage = Damage; // 직업의 데미지를 발사체에 전달
		Projectile->SetInitialSpeed(Speed);
	}
	return Projectile;
}
