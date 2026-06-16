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

void UJobComponent::InitializeForOwner(ACharacter* Owner)
{
	OwnerCharacter = Owner;
	EquipWeapon(); // 직업 무기를 소유자 무기 슬롯에 끼운다
}

void UJobComponent::EquipWeapon()
{
	// 무기 메시 교체는 플레이어의 무기 슬롯(ChildActor) 시스템 전용이다.
	// 동료(ACompanion)는 자기 BP 메시를 그대로 쓰므로 여기서 건드리지 않는다.
	// WeaponMesh가 null이면 무기가 숨겨진다(예: 지팡이 없는 마법사).
	if (AMyPlayer* Player = Cast<AMyPlayer>(OwnerCharacter))
	{
		Player->SetWeaponMesh(WeaponMesh);
	}
}

void UJobComponent::Attack()
{
	// 기본 동작: 공격 몽타주 재생. 몽타주의 Notify가 OnAttackNotify()를 부른다.
	if (OwnerCharacter && AttackMontage)
	{
		OwnerCharacter->PlayAnimMontage(AttackMontage);
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
	if (OwnerCharacter && AttackSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			OwnerCharacter,
			AttackSound,
			OwnerCharacter->GetActorLocation(),
			1.0f,
			1.0f
		);
	}
}

AProjectile* UJobComponent::SpawnProjectileForward(TSubclassOf<AProjectile> ProjectileClass, float Speed, float MuzzleOffset, float MuzzleHeight)
{
	if (!OwnerCharacter || !ProjectileClass)
	{
		return nullptr;
	}

	UWorld* World = OwnerCharacter->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const FVector Forward = OwnerCharacter->GetActorForwardVector();
	const FVector SpawnLocation =
		OwnerCharacter->GetActorLocation() + Forward * MuzzleOffset + FVector(0, 0, MuzzleHeight);
	const FRotator SpawnRotation = Forward.Rotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerCharacter;
	SpawnParams.Instigator = OwnerCharacter; // ACharacter는 APawn이라 Instigator로 사용 가능
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AProjectile* Projectile = World->SpawnActor<AProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (Projectile)
	{
		Projectile->Damage = Damage; // 직업의 데미지를 발사체에 전달
		Projectile->bDrawDebug = bDebugProjectileRange; // 사거리/적중범위 디버그 표시 여부 전달
		Projectile->SetInitialSpeed(Speed);
	}
	return Projectile;
}
