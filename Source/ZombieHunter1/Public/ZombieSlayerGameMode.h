#pragma once
#include <vector>
#define MAX_ENEMY_SIZE 5
#define MAX_COIN_SIZE 10

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Characters/Enemy.h"
#include "Coin.h"
#include "Characters/MyPlayer.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/TargetPoint.h"  // ATargetPoint 클래스 정의

#include "ZombieSlayerGameMode.generated.h"

/**
 * 
 */
UCLASS()
class ZOMBIEHUNTER1_API AZombieSlayerGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
	// UPROPERTY 필수 — 안 붙이면 적/코인이 Destroy될 때 이 raw 포인터가 댕글링이 되어
	// SpawnEnemy 등에서 역참조 시 크래시(액세스 위반)가 난다. UPROPERTY면 파괴 시 자동으로 null이 됨.
	UPROPERTY()
	TArray<AEnemy*> enemyPool;
	UPROPERTY()
	TArray<ACoin*> coinPool;
	
	int enemy_size = 0;
	int coin_size = 0;

	void initEnemy(int index);
	void initCoin(int index);

	void spawn();
	void spawnCoin();

	void init();

public:
	virtual void StartPlay() override;

	UPROPERTY()
	FTimerHandle SpawnTimerHandle; //타이머 설정 변수

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawners")
	TArray<ATargetPoint*> Spawners;


	UPROPERTY(EditDefaultsOnly, Category = "Enemy")
	TSubclassOf<AEnemy> EnemyClass;

	UPROPERTY(EditDefaultsOnly, Category = "Coin")
	TSubclassOf<AActor> CoinClass;

	//////////////////////////////////////////////////////////////////////////
	// 리쉬(leash) — 무한맵에서 뒤처져 청크 언로드로 지형을 잃는 적을 회수해 재배치한다.

	/** 플레이어에서 이 거리(cm) 이상 멀어진 적은 플레이어 근처로 재배치.
	 *  청크 언로드 경계(ViewRadiusInChunks × ChunkSize ≈ 6000)보다 안쪽이어야 낙하 전에 회수된다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Leash")
	float LeashDistance = 4500.0f;

	/** 이 Z(cm) 아래로 떨어진 적은 어떤 이유로든 즉시 회수 (리쉬가 놓친 케이스의 안전망) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Leash")
	float FallZThreshold = -500.0f;

	/** 리쉬 검사 주기(초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Leash")
	float LeashCheckInterval = 1.0f;

	/** 재배치 시 플레이어 주변 NavMesh에서 위치를 찾는 반경(cm) — SpawnEnemy와 동일 방식 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Leash")
	float LeashRespawnRadius = 500.0f;

	FTimerHandle LeashTimerHandle;

	/** 너무 멀어졌거나 낙하한 적을 플레이어 근처 NavMesh 위로 재배치 (LeashCheckInterval마다 호출) */
	void RecycleFarEnemies();

	UFUNCTION(BlueprintCallable)
	void SpawnEnemy();

	UFUNCTION(BlueprintCallable)
	void DieEnemy(int index);

	UFUNCTION(BlueprintCallable)
	void DestroyCoin(int index);

};
