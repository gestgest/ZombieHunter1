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

	UFUNCTION(BlueprintCallable)
	void SpawnEnemy();

	UFUNCTION(BlueprintCallable)
	void DieEnemy(int index);

	UFUNCTION(BlueprintCallable)
	void DestroyCoin(int index);

};
