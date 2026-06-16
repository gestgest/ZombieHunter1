// Fill out your copyright notice in the Description page of Project Settings.

#include "Jobs/JobComponent.h"
#include "MyPlayer.h"
#include "Projectiles/Projectile.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Engine.h" //GEngine 소켓 경고

UJobComponent::UJobComponent()
{
	// 직업 컴포넌트는 Tick이 필요 없다 (공격은 플레이어가 타이밍을 호출).
	PrimaryComponentTick.bCanEverTick = false;
}

void UJobComponent::InitializeForOwner(AMyPlayer* Player)
{
	OwnerPlayer = Player;
	EquipWeapon(); // 직업 무기를 손 소켓에 붙인다
}

void UJobComponent::EquipWeapon()
{
	if (!OwnerPlayer || !WeaponMesh)
	{
		return; // 무기가 없는 직업(또는 에셋 미지정)이면 그냥 통과
	}

	USkeletalMeshComponent* CharMesh = OwnerPlayer->GetMesh();
	if (!CharMesh)
	{
		return;
	}

	// 무기 메시 컴포넌트를 런타임 생성해 캐릭터 손 소켓에 부착.
	// 무기는 손에 들려만 있으면 되므로 충돌은 끈다(타격 판정은 직업 로직이 따로 처리).
	WeaponMeshComp = NewObject<USkeletalMeshComponent>(OwnerPlayer);
	if (!WeaponMeshComp)
	{
		return;
	}
	WeaponMeshComp->SetSkeletalMesh(WeaponMesh);
	WeaponMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMeshComp->RegisterComponent();
	WeaponMeshComp->AttachToComponent(
		CharMesh,
		FAttachmentTransformRules::SnapToTargetIncludingScale,
		WeaponSocket);

	// 소켓이 스켈레톤에 없으면 컴포넌트 원점(발밑)에 붙어 무기가 엉뚱한 곳에 보인다 → 경고.
	if (!CharMesh->DoesSocketExist(WeaponSocket) && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Orange,
			FString::Printf(TEXT("[Weapon] 소켓 '%s' 없음 - 캐릭터 스켈레톤에 소켓을 추가하세요"),
				*WeaponSocket.ToString()));
	}
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
		Projectile->bDrawDebug = bDebugProjectileRange; // 사거리/적중범위 디버그 표시 여부 전달
		Projectile->SetInitialSpeed(Speed);
	}
	return Projectile;
}
