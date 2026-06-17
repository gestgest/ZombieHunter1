// Fill out your copyright notice in the Description page of Project Settings.

#include "CombatCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"

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

void ACombatCharacter::OnMontageNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& Payload)
{
	HandleAttackNotify(NotifyName);
}
