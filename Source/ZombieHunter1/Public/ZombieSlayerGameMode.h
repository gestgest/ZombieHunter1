#pragma once
#include <vector>
#define MAX_ENEMY_SIZE 5
#define MAX_COIN_SIZE 10

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Enemy.h"
#include "MyPlayer.h"
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
	
	TArray<AEnemy*> enemyPool;
	TArray<AActor*> coinPool;
	
	int enemy_size = 0;
	int coin_size = 0;

	void initEnemy();
	void initCoin();
	void spawnCoin();

	void init();

public:
	virtual void StartPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawners")
	TArray<ATargetPoint*> Spawners;


	UPROPERTY(EditDefaultsOnly, Category = "Enemy")
	TSubclassOf<AEnemy> EnemyClass;

	UPROPERTY(EditDefaultsOnly, Category = "Coin")
	TSubclassOf<AActor> CoinClass;

	UFUNCTION(BlueprintCallable)
	void SpawnEnemy();

};
