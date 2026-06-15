// Fill out your copyright notice in the Description page of Project Settings.

#include "JobComponent.h"
#include "MyPlayer.h"
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
