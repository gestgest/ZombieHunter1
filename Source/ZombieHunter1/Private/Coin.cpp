// Fill out your copyright notice in the Description page of Project Settings.


#include "Coin.h"

// Sets default values
ACoin::ACoin()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ACoin::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ACoin::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ACoin::setID(int value)
{
	this->coin_id = value;
}

int ACoin::GetID()
{
	return coin_id;
}

void ACoin::SetCanGet(bool value)
{
	this->canGet = value;
}

bool ACoin::GetCanGet()
{
	return canGet;
}

