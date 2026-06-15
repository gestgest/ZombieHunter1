// Fill out your copyright notice in the Description page of Project Settings.

#include "Projectile.h"
#include "Enemy.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	// 충돌 구체를 루트로. Pawn(적)과 겹침을 감지한다.
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(20.0f);
	CollisionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	RootComponent = CollisionSphere;

	// 시각 메시 (BP에서 화살 메시를 지정). 충돌은 구체가 담당하므로 메시는 충돌 끔.
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 직선 비행
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 2000.0f;
	ProjectileMovement->MaxSpeed = 2000.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 0.0f; // 탑다운: 중력 영향 없음

	// 5초 뒤 자동 소멸 (빗나간 화살 정리)
	InitialLifeSpan = 5.0f;
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AProjectile::OnSphereOverlap);
}

void AProjectile::SetInitialSpeed(float Speed)
{
	if (ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = Speed;
		ProjectileMovement->MaxSpeed = Speed;
		// 이미 속도가 설정돼 발사된 경우를 대비해 현재 진행 방향으로 속도 갱신
		ProjectileMovement->Velocity = GetActorForwardVector() * Speed;
	}
}

void AProjectile::OnSphereOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	AEnemy* HitEnemy = Cast<AEnemy>(OtherActor);
	if (!HitEnemy)
	{
		return; // 적이 아니면 통과 (벽/플레이어 등 무시)
	}

	HitEnemy->AddHP(-Damage);

	const FVector Force = GetActorForwardVector() * KnockbackForce + FVector(0, 0, 100);
	HitEnemy->LaunchCharacter(Force, false, false);

	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, GetActorLocation());
	}

	Destroy();
}
