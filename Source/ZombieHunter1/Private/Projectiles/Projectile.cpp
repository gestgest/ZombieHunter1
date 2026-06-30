#include "Projectiles/Projectile.h"
#include "Characters/Enemy.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Engine/OverlapResult.h" // FOverlapResult (범위 폭발 오버랩 결과)
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h" // 디버그 범위 시각화

//투사체
AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true; // 디버그 그리기용. bDrawDebug=false면 Tick에서 즉시 반환.

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

	// 디버그 사거리 라인 기준점 기록 (발사 지점/방향)
	SpawnLocation = GetActorLocation();
	SpawnForward = GetActorForwardVector();
}

void AProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bDrawDebug)
	{
		return;
	}
	

	// 비행 중 충돌 구체(실제 적중 판정 범위)를 매 프레임 그린다.
	DrawDebugSphere(GetWorld(), GetActorLocation(),
		CollisionSphere->GetScaledSphereRadius(), 12, FColor::Cyan, false, -1.0f, 0, 1.0f);

	// 폭발형이면 적중 시 터질 범위를 미리 노란 구체로 보여준다.
	if (ExplosionRadius > 0.0f)
	{
		DrawDebugSphere(GetWorld(), GetActorLocation(),
			ExplosionRadius, 16, FColor::Yellow, false, -1.0f, 0, 1.0f);
	}

	// 최대 사거리 = 속도 × 수명. 발사 지점에서 진행 방향으로 라인 + 끝점 표시.
	const float Speed = ProjectileMovement ? ProjectileMovement->MaxSpeed : 0.0f;
	const float MaxRange = Speed * InitialLifeSpan;
	if (MaxRange > 0.0f)
	{
		const FVector EndPoint = SpawnLocation + SpawnForward * MaxRange;
		DrawDebugLine(GetWorld(), SpawnLocation, EndPoint, FColor::Green, false, -1.0f, 0, 2.0f);
		DrawDebugSphere(GetWorld(), EndPoint, 20.0f, 8, FColor::Green, false, -1.0f, 0, 1.0f);
	}
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

	// 범위 폭발: 적중 지점 주변의 모든 적에게 피해 (마법사 파이어볼 등)
	if (ExplosionRadius > 0.0f)
	{
		TArray<FOverlapResult> Overlaps;
		FCollisionShape Sphere = FCollisionShape::MakeSphere(ExplosionRadius);
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);

		GetWorld()->OverlapMultiByChannel(
			Overlaps, GetActorLocation(), FQuat::Identity, ECC_Pawn, Sphere, Params);

		if (bDrawDebug)
		{
			// 실제로 터진 범위를 1.5초간 빨간 구체로 남겨서 확인.
			DrawDebugSphere(GetWorld(), GetActorLocation(),
				ExplosionRadius, 16, FColor::Red, false, 1.5f, 0, 2.0f);
		}

		TSet<AEnemy*> Damaged;
		for (const FOverlapResult& O : Overlaps)
		{
			AEnemy* Enemy = Cast<AEnemy>(O.GetActor());
			if (Enemy && !Damaged.Contains(Enemy))
			{
				Damaged.Add(Enemy);
				ApplyHit(Enemy);
			}
		}
	}
	else
	{
		// 단일 대상 (화살 등)
		ApplyHit(HitEnemy);
	}

	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, GetActorLocation());
	}

	Destroy();
}

void AProjectile::ApplyHit(AEnemy* Enemy)
{
	if (!Enemy)
	{
		return;
	}

	Enemy->AddHP(-Damage);

	// 발사체(폭발 중심)에서 적을 향하는 수평 방향으로 넉백. 같은 위치면 진행 방향으로.
	FVector Dir = Enemy->GetActorLocation() - GetActorLocation();
	Dir.Z = 0.0f;
	if (!Dir.Normalize())
	{
		Dir = GetActorForwardVector();
	}

	const FVector Force = Dir * KnockbackForce + FVector(0, 0, 100);
	Enemy->LaunchCharacter(Force, false, false);
}
