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

    // 리쉬 검사: 뒤처져 지형을 잃(을 뻔하)는 적을 주기적으로 회수
    GetWorldTimerManager().SetTimer(
        LeashTimerHandle,
        this,
        &AZombieSlayerGameMode::RecycleFarEnemies,
        LeashCheckInterval,
        true
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
#if WITH_EDITOR
        newEnemy->SetFolderPath(TEXT("Spawned/Enemies")); // 아웃라이너 정리용 (에디터 전용)
#endif
        enemyPool.Add(newEnemy);  // 풀에 추가
        newEnemy->EnterPoolDormancy();  // 숨김 + 콜리전/이동/틱 정지 (숨김만으론 중력에 낙하)
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
#if WITH_EDITOR
        newCoin->SetFolderPath(TEXT("Spawned/Coins")); // 아웃라이너 정리용 (에디터 전용)
#endif
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
        // 파괴됐거나 무효한 풀 엔트리는 건너뛴다 (댕글링 포인터 역참조 = 크래시 방지).
        if (!IsValid(enemyPool[i]))
        {
            continue;
        }
        if (enemyPool[i]->IsHidden())
        {
            break;
        }
    }
    //Max (또는 쓸 수 있는 적이 없음)
    if (i >= enemyPool.Num())
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
        enemy->WakeFromPool();  // 숨김 해제 + 콜리전/이동/틱 복구
        enemy->SetHP(5);        // 죽었던 적이면 부활 전환(OnRevive)까지 발동
    }
    else
    {
        return;
    }

    enemy_size++;
}

// 무한맵 리쉬: 플레이어에서 너무 멀어졌거나(청크 언로드로 지형 상실 직전) 이미 낙하한 적을
// 플레이어 근처 NavMesh 위로 재배치한다. 적이 사라지는 대신 진행 방향에서 다시 나타나 압박 유지.
void AZombieSlayerGameMode::RecycleFarEnemies()
{
    AMyPlayer* myPlayer =
        Cast<AMyPlayer>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
    if (!myPlayer)
    {
        return;
    }

    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!NavSys)
    {
        return;
    }

    const FVector playerLocation = myPlayer->GetActorLocation();
    const FVector upVector(0, 0, 90.0f);

    for (AEnemy* enemy : enemyPool)
    {
        // 풀 대기 중이거나 죽어서 사망 연출 중인 적은 건드리지 않는다
        if (!IsValid(enemy) || enemy->IsHidden() || enemy->IsDead)
        {
            continue;
        }

        const FVector enemyLocation = enemy->GetActorLocation();
        const bool bTooFar  = FVector::Dist2D(enemyLocation, playerLocation) > LeashDistance;
        const bool bFalling = enemyLocation.Z < FallZThreshold;
        if (!bTooFar && !bFalling)
        {
            continue;
        }

        // 플레이어 주변 도달 가능한 위치로 재배치. 실패하면 이번 주기는 넘기고 다음에 재시도.
        FNavLocation resultLocation;
        if (NavSys->GetRandomReachablePointInRadius(playerLocation, LeashRespawnRadius, resultLocation))
        {
            enemy->TeleportForLeash(resultLocation.Location + upVector);
        }
    }
}


void AZombieSlayerGameMode::spawnCoin()
{
    int i = 0;

    for (; i < coinPool.Num(); i++)
    {
        if (!IsValid(coinPool[i]))
        {
            continue;
        }
        if (coinPool[i]->IsHidden())
        {
            break;
        }
    }

    //Max (또는 쓸 수 있는 코인이 없음)
    if (i >= coinPool.Num())
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
    if (enemyPool.IsValidIndex(index) && IsValid(enemyPool[index]))
    {
        enemyPool[index]->EnterPoolDormancy();  // 풀로 반납 (숨김 + 콜리전/이동/틱 정지)
    }
}

//signal을 받아야지 될듯
void AZombieSlayerGameMode::DestroyCoin(int index)
{
    coin_size--;
    if (coinPool.IsValidIndex(index) && IsValid(coinPool[index]))
    {
        coinPool[index]->SetCanGet(false);
        coinPool[index]->SetActorHiddenInGame(true);  // 비활성화
    }
}
