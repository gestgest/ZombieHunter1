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

	// 유니티의 OntriggerEnter, Exit 함수 설정
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

	if (bPlayerInside && bActive && IsValid(CurrentPlayer))
	{
		// 발판 위에 서 있으면 일정 간격마다 "돈을 소비"해서 게이지를 채운다.
		PaymentTimer += DeltaTime;
		if (PaymentTimer >= PaymentInterval)
		{
			PaymentTimer = 0.0f;

			// 마지막 한 칸은 남은 금액(MaxMoney - PaidMoney)만 받아 초과 결제를 막는다.
			const int32 Payment = FMath::Min(MoneyPerPayment, MaxMoney - PaidMoney);

			if (Payment > 0 && CurrentPlayer->TrySpendMoney(Payment))
			{
				// 결제 성공 → 누적 돈 증가, 게이지는 PaidMoney / MaxMoney로 계산.
				PaidMoney += Payment;
				Progress = FMath::Clamp((float)PaidMoney / (float)MaxMoney, 0.0f, 1.0f);
				OnProgressChanged(Progress);

				if (PaidMoney >= MaxMoney)
				{
					CompleteZone();
				}
			}
			else
			{
				// 돈이 부족하면 게이지를 올리지 않고 그대로 둔다(이미 낸 돈만큼은 유지).
				OnInsufficientFunds();
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(7001, 1.0f, FColor::Red,
						FString::Printf(TEXT("[SpawnZone] 돈 부족! (%d원 필요)"), MoneyPerPayment));
				}
			}
		}
	}
	else
	{
		// 발판에서 벗어나면 결제 타이머만 초기화한다. 이미 채운 게이지는 유지(낸 돈이 아까우니까).
		PaymentTimer = 0.0f;
	}

	if (bShowDebugGauge)
	{
		DrawDebugGauge();
	}
}

void ACompanionSpawnZone::CompleteZone()
{
	AMyPlayer* Recruiter = CurrentPlayer;

	// 게이지/누적 돈 리셋 후 동료 소환(C키 섭외와 동일 로직 재사용).
	PaidMoney = 0;
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


//스폰존에 들어간다면
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

//스폰존에 나간다면
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

// debug : 그리는 함수
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
