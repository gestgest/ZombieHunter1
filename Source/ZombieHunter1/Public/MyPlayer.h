// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MyCanvas.h" 
#include "MyPlayer.generated.h"


UCLASS()
class ZOMBIEHUNTER1_API AMyPlayer : public ACharacter
{
	GENERATED_BODY()
	UMyCanvas* CanvasWidget; 

	bool checkDead();
public:
	// Sets default values for this character's properties
	AMyPlayer();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// CheckDeath 함수가 리턴값이 없다면
	UFUNCTION(BlueprintImplementableEvent)
	void CheckDeath(bool isDead);


	UFUNCTION(BlueprintCallable)
	void SetCanvasWidget(UMyCanvas* cw);

	// 블루프린트에서 읽고 쓸 수 있는 Money 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Stats")
	int32 Money;
	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Stats")
	int32 HP;

	//AddCoin
	UFUNCTION(BlueprintCallable)
	void AddMoney();

	UFUNCTION(BlueprintCallable)
	void AddHP(int add_hp);

	UFUNCTION(BlueprintCallable)
	void SetHP(int new_hp);




};
