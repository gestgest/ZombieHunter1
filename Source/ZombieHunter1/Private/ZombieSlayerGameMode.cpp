// Fill out your copyright notice in the Description page of Project Settings.


#include "ZombieSlayerGameMode.h"

void AZombieSlayerGameMode::StartPlay()
{
    Super::StartPlay();
    AZombieSlayerGameMode::init();
}

void AZombieSlayerGameMode::initEnemy()
{
    FVector SpawnLocation = FVector(100, 0, 50);
    FRotator SpawnRotation = FRotator::ZeroRotator;

    // SpawnParameters 설정
    FActorSpawnParameters spawnParams;
    spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    // Enemy 스폰
    AEnemy* newEnemy = GetWorld()->SpawnActor<AEnemy>(
        EnemyClass,
        FVector(100, 0, 50),
        SpawnRotation,
        spawnParams
    );

    if (newEnemy)
    {
        enemyPool.Add(newEnemy);  // 풀에 추가
        newEnemy->SetActorHiddenInGame(true);  // 비활성화
    }
    else {
    }
}

void AZombieSlayerGameMode::initCoin()
{
    AActor* NewCoin = GetWorld()->SpawnActor<AActor>(
        CoinClass,
        FVector(100, 0, 50),
        FRotator::ZeroRotator
    );

    if (NewCoin)
    {
        coinPool.Add(NewCoin);
        NewCoin->SetActorHiddenInGame(true);  // 비활성화
    }
}

void AZombieSlayerGameMode::SpawnEnemy()
{
    if (MAX_ENEMY_SIZE <= enemy_size)
        return;

    AEnemy *enemy = enemyPool[enemy_size];
    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
 
    float radius = 500;

    AMyPlayer* myPlayer =
        Cast<AMyPlayer>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));

    FVector playerLocation = myPlayer->GetActorLocation();
    FNavLocation resultLocation;



    // 플레이어 주변 Radius 내 랜덤 도달가능 위치 찾기
    bool bSuccess = NavSys->GetRandomReachablePointInRadius(playerLocation, radius, resultLocation);
    

    if (bSuccess)
    {
        enemy->SetActorLocation(resultLocation.Location);
        enemy->SetActorHiddenInGame(false);
    }

    enemy_size++;
}

void AZombieSlayerGameMode::spawnCoin()
{
}

void AZombieSlayerGameMode::init()
{
    enemy_size = 0;
    coin_size = 0;


    for (int i = 0; i < MAX_ENEMY_SIZE; i++)
    {
        initEnemy();
    }
    for (int i = 0; i < MAX_COIN_SIZE; i++)
    {
        initCoin();
    }
}
