// Fill out your copyright notice in the Description page of Project Settings.


#include "ZombieSlayerGameMode.h"


//begin
void AZombieSlayerGameMode::StartPlay()
{
    Super::StartPlay();
    AZombieSlayerGameMode::init();

    // 5초마다 SpawnEnemyTimer 함수 반복 호출
    GetWorldTimerManager().SetTimer(
        SpawnTimerHandle,           // 타이머 핸들
        this,                        // 호출할 객체
        &AZombieSlayerGameMode::spawn,  // 호출할 함수
        5.0f,                        // 간격 (5초)
        true                         // 반복 여부 (true = 반복)
    );
}

void AZombieSlayerGameMode::init()
{
    enemy_size = 0;
    coin_size = 0;


    for (int i = 0; i < MAX_ENEMY_SIZE; i++)
    {
        initEnemy(i);
    }
    for (int i = 0; i < MAX_COIN_SIZE; i++)
    {
        initCoin(i);
    }
}


void AZombieSlayerGameMode::initEnemy(int index)
{
    FVector SpawnLocation = FVector(0, 0, 90);
    FRotator SpawnRotation = FRotator::ZeroRotator;

    // SpawnParameters 설정
    FActorSpawnParameters spawnParams;
    spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    // Enemy 스폰
    AEnemy* newEnemy = GetWorld()->SpawnActor<AEnemy>(
        EnemyClass,
        SpawnLocation,
        SpawnRotation,
        spawnParams
    );

    newEnemy->SetID(index);

    if (newEnemy)
    {
        enemyPool.Add(newEnemy);  // 풀에 추가

        newEnemy->SetActorHiddenInGame(true);  // 비활성화
    }
}

void AZombieSlayerGameMode::initCoin(int index)
{
    FVector SpawnLocation = FVector(0, 0, 90);
    FRotator SpawnRotation = FRotator::ZeroRotator;

    // SpawnParameters 설정
    FActorSpawnParameters spawnParams;
    spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ACoin* newCoin = GetWorld()->SpawnActor<ACoin>(
        CoinClass,
        SpawnLocation,
        SpawnRotation,
        spawnParams
    );

    newCoin->setID(index);
    newCoin->SetCanGet(false);

    if (newCoin)
    {
        coinPool.Add(newCoin);
        newCoin->SetActorHiddenInGame(true);  // 비활성화
    }
}
void AZombieSlayerGameMode::spawn()
{
    SpawnEnemy();
    spawnCoin();
}

void AZombieSlayerGameMode::SpawnEnemy()
{
    int i = 0;

    for (; i < enemyPool.Num(); i++)
    {
        if (enemyPool[i]->IsHidden())
        {
            break;
        }
    }
    //Max
    if (i == enemyPool.Num())
    {
        return;
    }

    AEnemy *enemy = enemyPool[i];

    if (!enemy->GetController())
    {
        enemy->SetAIController();
    }

    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
 
    float radius = 500;


    AMyPlayer* myPlayer =
        Cast<AMyPlayer>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));

    FVector playerLocation = myPlayer->GetActorLocation();
    FNavLocation resultLocation;

    // 플레이어 주변 Radius 내 랜덤 도달가능 위치 찾기
    bool bSuccess = NavSys->GetRandomReachablePointInRadius(playerLocation, radius, resultLocation);

    FVector upVector(0, 0, 90.0f);

    if (bSuccess)
    {
        enemy->SetActorLocation(resultLocation.Location + upVector);
        enemy->SetActorHiddenInGame(false);
        enemy->SetHP(5);
    }
    else
    {
        return;
    }

    enemy_size++;
}


void AZombieSlayerGameMode::spawnCoin()
{
    int i = 0;

    for (; i < coinPool.Num(); i++)
    {
        if (coinPool[i]->IsHidden())
        {
            break;
        }
    }

    //Max
    if (i == coinPool.Num())
    {
        return;
    }

    ACoin* coin = coinPool[i];
    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

    float radius = 500;


    AMyPlayer* myPlayer =
        Cast<AMyPlayer>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));

    FVector playerLocation = myPlayer->GetActorLocation();
    FNavLocation resultLocation;

    // 플레이어 주변 Radius 내 랜덤 도달가능 위치 찾기
    bool bSuccess = NavSys->GetRandomReachablePointInRadius(playerLocation, radius, resultLocation);

    FVector upVector(0, 0, 90.0f);
    
    if (bSuccess)
    {
        coin->SetActorLocation(resultLocation.Location + upVector);
        coin->SetActorEnableCollision(true);
        coin->SetActorHiddenInGame(false);
        coin->SetCanGet(true);
    }
    else
    {
        return;
    }

    coin_size++;
}

//signal을 받아야지 될듯
void AZombieSlayerGameMode::DieEnemy(int index)
{
    enemy_size--;
    //enemyPool[index]->SetActorEnableCollision(false);
    // => 너무 바깥 범위를 넘어간듯
    enemyPool[index]->SetActorHiddenInGame(true);  // 비활성화
}

//signal을 받아야지 될듯
void AZombieSlayerGameMode::DestroyCoin(int index)
{
    coin_size--;
    //enemyPool[index]->SetActorEnableCollision(false);
    // => 너무 바깥 범위를 넘어간듯
    coinPool[index]->SetCanGet(false);
    coinPool[index]->SetActorHiddenInGame(true);  // 비활성화
}
