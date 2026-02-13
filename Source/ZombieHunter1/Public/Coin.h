// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Coin.generated.h"

UCLASS()
class ZOMBIEHUNTER1_API ACoin : public AActor
{
	GENERATED_BODY()
	
	int coin_id;
	bool canGet;
public:	
	// Sets default values for this actor's properties
	ACoin();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void setID(int id);

	UFUNCTION(BlueprintCallable)
	int GetID();

	void SetCanGet(bool value);

	UFUNCTION(BlueprintCallable)
	bool GetCanGet();
};
