// Fill out your copyright notice in the Description page of Project Settings.


#include "ZombieSlayerGameMode.h"

void AZombieSlayerGameMode::StartPlay()
{
    Super::StartPlay();
    AZombieSlayerGameMode::init();
}

void AZombieSlayerGameMode::initEnemy()
{
    enemy_size = 0;

    FVector SpawnLocation = FVector(100, 0, 50);
    FRotator SpawnRotation = FRotator::ZeroRotator;

    // Enemy 스폰
    AEnemy* newEnemy = GetWorld()->SpawnActor<AEnemy>(
        EnemyClass,
        FVector(100, 0, 50),
        SpawnRotation
    );

    if (newEnemy)
    {
        enemyPool.Add(newEnemy);  // 풀에 추가
        newEnemy->SetActorHiddenInGame(true);  // 비활성화
    }
}

void AZombieSlayerGameMode::initCoin()
{
    coin_size = 0;
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

void AZombieSlayerGameMode::spawnEnemy()
{
    AEnemy *enemy = enemyPool[enemy_size];
    

    enemy_size++;


}

void AZombieSlayerGameMode::spawnCoin()
{
}

void AZombieSlayerGameMode::init()
{
    for (int i = 0; i < MAX_ENEMY_SIZE; i++)
    {
        initEnemy();
    }
    for (int i = 0; i < MAX_COIN_SIZE; i++)
    {
        initCoin();
    }
}
