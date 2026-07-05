// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/EnemyHPBarWidget.h"
#include "Components/ProgressBar.h"

void UEnemyHPBarWidget::SetHPPercent(float Percent)
{
	if (HPBar)
	{
		HPBar->SetPercent(FMath::Clamp(Percent, 0.0f, 1.0f));
	}
}
