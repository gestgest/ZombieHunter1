// Fill out your copyright notice in the Description page of Project Settings.

#include "CompanionSpawnZone.h"
#include "Characters/MyPlayer.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"

ACompanionSpawnZone::ACompanionSpawnZone()
{
	PrimaryActorTick.bCanEverTick = true;

	// 박스 트리거를 루트로. 기본 크기는 사람이 올라설 만한 발판 정도.
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	RootComponent = TriggerBox;
	TriggerBox->SetBoxExtent(FVector(150.0f, 150.0f, 100.0f));
	TriggerBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	TriggerBox->SetGenerateOverlapEvents(true);

	// 시각화용 발판 메시(메시는 BP에서 지정). 충돌은 끔 — 트리거만 충돌을 담당.
	PadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PadMesh"));
	PadMesh->SetupAttachment(RootComponent);
	PadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ACompanionSpawnZone::BeginPlay()
{
	Super::BeginPlay();

	if (TriggerBox)
	{
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ACompanionSpawnZone::OnTriggerBeginOverlap);
		TriggerBox->OnComponentEndOverlap.AddDynamic(this, &ACompanionSpawnZone::OnTriggerEndOverlap);
	}
}

void ACompanionSpawnZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 완성 후 쿨다운 진행
	if (CooldownRemaining > 0.0f)
	{
		CooldownRemaining = FMath::Max(0.0f, CooldownRemaining - DeltaTime);
	}

	// 추적 중인 플레이어가 죽었거나 사라졌으면 정리
	if (!IsValid(CurrentPlayer))
	{
		CurrentPlayer = nullptr;
		bPlayerInside = false;
	}

	const bool bActive = !bConsumed && CooldownRemaining <= 0.0f;
	const float OldProgress = Progress;

	if (bPlayerInside && bActive)
	{
		// 서 있으면 채운다.
		Progress = FMath::Min(1.0f, Progress + DeltaTime / FMath::Max(0.1f, FillDuration));

		if (Progress >= 1.0f)
		{
			CompleteZone();
		}
	}
	else if (Progress > 0.0f && DrainRateScale > 0.0f)
	{
		// 비었으면 줄인다(채우는 속도 × DrainRateScale).
		const float DrainPerSec = (1.0f / FMath::Max(0.1f, FillDuration)) * DrainRateScale;
		Progress = FMath::Max(0.0f, Progress - DeltaTime * DrainPerSec);
	}

	// 게이지가 실제로 바뀌었을 때만 BP 이벤트 통지
	if (!FMath::IsNearlyEqual(OldProgress, Progress))
	{
		OnProgressChanged(Progress);
	}

	if (bShowDebugGauge)
	{
		DrawDebugGauge();
	}
}

void ACompanionSpawnZone::CompleteZone()
{
	AMyPlayer* Recruiter = CurrentPlayer;

	// 게이지 리셋 후 동료 소환(C키 섭외와 동일 로직 재사용).
	Progress = 0.0f;
	OnProgressChanged(0.0f);

	if (IsValid(Recruiter))
	{
		Recruiter->RecruitCompanion();
	}

	OnZoneCompleted(Recruiter);

	// 다음 작동 제어: 일회성이면 소비, 아니면 쿨다운.
	if (bOneShot)
	{
		bConsumed = true;
	}
	else
	{
		CooldownRemaining = Cooldown;
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Cyan,
			TEXT("[SpawnZone] 게이지 완료 - 동료 소환!"));
	}
}

void ACompanionSpawnZone::OnTriggerBeginOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*Sweep*/)
{
	AMyPlayer* Player = Cast<AMyPlayer>(OtherActor);
	if (!Player)
	{
		return;
	}

	CurrentPlayer = Player;
	bPlayerInside = true;
}

void ACompanionSpawnZone::OnTriggerEndOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/)
{
	if (OtherActor != CurrentPlayer)
	{
		return;
	}

	// 추적하던 플레이어가 구역을 떠남 — 다른 플레이어가 아직 안에 있으면 그 사람으로 승계.
	CurrentPlayer = nullptr;
	bPlayerInside = false;

	if (TriggerBox)
	{
		TArray<AActor*> Overlapping;
		TriggerBox->GetOverlappingActors(Overlapping, AMyPlayer::StaticClass());
		for (AActor* A : Overlapping)
		{
			if (AMyPlayer* P = Cast<AMyPlayer>(A))
			{
				CurrentPlayer = P;
				bPlayerInside = true;
				break;
			}
		}
	}
}

void ACompanionSpawnZone::DrawDebugGauge()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 발판 위에 떠 있는 가로 게이지 바: 회색 배경 + 진행도만큼 초록 채움.
	const FVector Base = GetActorLocation() + FVector(0, 0, 220.0f);
	const float HalfWidth = 120.0f;
	const FVector Left = Base + FVector(-HalfWidth, 0, 0);
	const FVector Right = Base + FVector(HalfWidth, 0, 0);
	const FVector FillRight = Left + (Right - Left) * FMath::Clamp(Progress, 0.0f, 1.0f);

	DrawDebugLine(World, Left, Right, FColor(60, 60, 60), false, -1.0f, 0, 8.0f);
	DrawDebugLine(World, Left, FillRight, FColor::Green, false, -1.0f, 0, 8.0f);
}
