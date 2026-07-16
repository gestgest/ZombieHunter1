// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/CombatCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/WidgetComponent.h" //머리 위 HP 바
#include "UI/EnemyHPBarWidget.h"

ACombatCharacter::ACombatCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ACombatCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 공격 몽타주 Notify → HandleAttackNotify (서브클래스가 실제 공격을 처리).
	// 플레이어/동료/적이 똑같이 쓰던 배선을 여기로 모았다.
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		if (UAnimInstance* Anim = MeshComp->GetAnimInstance())
		{
			Anim->OnPlayMontageNotifyBegin.AddDynamic(this, &ACombatCharacter::OnMontageNotifyBegin);
		}
	}
}

void ACombatCharacter::AddHP(int32 add_hp)
{
	SetHP(HP + add_hp);
}

void ACombatCharacter::SetHP(int32 new_hp)
{
	HP = new_hp;
	SetDead(HP <= 0);
	UpdateHPBar(); // 머리 위 바가 있는 캐릭터(적/동료)만 실제로 갱신된다
}

// 머리 위 HP 바 생성 — 반드시 서브클래스 생성자에서 호출(CreateDefaultSubobject 제약).
// 스크린 스페이스라 탑다운 카메라에서도 항상 화면을 향하고 크기가 일정하다.
void ACombatCharacter::CreateHPBarComponent()
{
	HPBarComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HPBar"));
	HPBarComponent->SetupAttachment(RootComponent);
	HPBarComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f)); // 캡슐(반높이 ~88) 위
	HPBarComponent->SetWidgetSpace(EWidgetSpace::Screen);
	HPBarComponent->SetDrawSize(FVector2D(100.0f, 12.0f));
	HPBarComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ACombatCharacter::UpdateHPBar()
{
	if (!HPBarComponent)
	{
		return; // 플레이어 등 바 없는 캐릭터
	}

	if (UEnemyHPBarWidget* Bar = Cast<UEnemyHPBarWidget>(HPBarComponent->GetWidget()))
	{
		Bar->SetHPPercent(MaxHP > 0 ? (float)HP / (float)MaxHP : 0.0f);
	}

	// 죽으면 시체 위에 바가 남지 않게 숨긴다. 풀 재사용/부활(SetHP>0) 시 다시 보인다.
	HPBarComponent->SetVisibility(!IsDead);
}

void ACombatCharacter::SetDead(bool bNewDead)
{
	const bool bWasDead = IsDead;
	IsDead = bNewDead;

	// 전환 시점에만 1회씩 — 죽음/부활 처리가 중복 발동하지 않게.
	if (bNewDead && !bWasDead)
	{
		OnDeath();
	}
	else if (!bNewDead && bWasDead)
	{
		OnRevive();
	}
}

UAnimMontage* ACombatCharacter::GetAttackMontageForJob(FName JobName) const
{
	// 이 캐릭터의 스켈레톤에 맞는 직업별 몽타주가 있으면 그걸, 없으면 단일 AttackMontage로 폴백.
	if (UAnimMontage* const* Found = JobAttackMontages.Find(JobName))
	{
		if (*Found)
		{
			return *Found;
		}
	}
	return AttackMontage;
}

void ACombatCharacter::OnMontageNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& Payload)
{
	HandleAttackNotify(NotifyName);
}
