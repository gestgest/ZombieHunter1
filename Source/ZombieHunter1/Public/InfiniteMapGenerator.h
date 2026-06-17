// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InfiniteMapGenerator.generated.h"

class UStaticMesh;
class UMaterialInterface;
class AStaticMeshActor;
class APawn;
class ANavMeshBoundsVolume;

/** 한 청크가 스폰한 액터 묶음 (언로드 시 한 번에 Destroy) */
USTRUCT()
struct FMapChunk
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedActors;
};

/**
 * 플레이어를 따라다니며 주변 청크를 동적으로 생성/제거하는 무한 맵 생성기.
 * 레벨에 하나 배치하고 Details 패널에서 바닥/장애물 메시를 지정해 사용한다.
 */
UCLASS()
class ZOMBIEHUNTER1_API AInfiniteMapGenerator : public AActor
{
	GENERATED_BODY()

public:
	AInfiniteMapGenerator();
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	//////////////////////////////////////////////////////////////////////////
	// [디버그]

	//켜면 청크 생성/갱신을 전부 멈춤. 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Debug")
	bool debugDisableGeneration = false;



	//////////////////////////////////////////////////////////////////////////
	// 기본 설정

	/** 청크 한 변의 길이(cm) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	float ChunkSize = 2000.f;

	/** 플레이어 기준 몇 청크까지 유지할지 (반경 R → (2R+1)^2 개 로드) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	int32 ViewRadiusInChunks = 3;

	/** 청크 갱신 주기(초). 매 프레임이 아니라 이 간격으로만 검사 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	float UpdateInterval = 0.25f;

	/** 켜면 NavMeshBoundsVolume를 플레이어 따라 옮겨, 무한맵 어디로 가도 그 주변에 NavMesh가 깔리게 한다.
	 *  (볼륨은 인보커 반경 + 여유를 덮을 정도면 충분 — 너무 거대하게 둘 필요 없음) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|Navigation")
	bool bFollowNavBounds = true;

	/** 전역 시드. 같은 시드+같은 청크 좌표면 항상 동일하게 생성됨 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	int32 GlobalSeed = 1337;

	//////////////////////////////////////////////////////////////////////////
	// 바닥

	/** 바닥 타일 메시 (예: LevelPrototyping/Meshes/SM_Cube) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|Floor")
	TObjectPtr<UStaticMesh> FloorMesh;

	/** 바닥에 덮어씌울 머티리얼(선택) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|Floor")
	TObjectPtr<UMaterialInterface> FloorMaterial;

	/** FloorMesh의 원본 XY 한 변 길이(cm). 엔진 큐브 계열은 보통 100 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|Floor")
	float FloorMeshBaseSize = 100.f;

	/** 바닥 두께(cm). 윗면이 Z=0에 오도록 배치됨 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|Floor")
	float FloorThickness = 20.f;

	//////////////////////////////////////////////////////////////////////////
	// 장애물

	/** 장애물 후보 메시들 (랜덤으로 골라 배치) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|Obstacles")
	TArray<TObjectPtr<UStaticMesh>> ObstacleMeshes;

	/** 장애물 메시의 원본 한 변 길이(cm). 바닥 위에 앉히기 위한 높이 계산용 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|Obstacles")
	float ObstacleMeshBaseSize = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|Obstacles")
	int32 MinObstaclesPerChunk = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|Obstacles")
	int32 MaxObstaclesPerChunk = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|Obstacles")
	float ObstacleMinScale = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|Obstacles")
	float ObstacleMaxScale = 2.0f;

	/** 장애물이 청크 가장자리에서 떨어질 여백(cm) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|Obstacles")
	float ChunkEdgeMargin = 150.f;

private:
	/** 현재 로드된 청크들 (좌표 → 스폰 액터들) */
	UPROPERTY()
	TMap<FIntPoint, FMapChunk> LoadedChunks;

	UPROPERTY()
	TObjectPtr<APawn> TrackedPawn;

	/** 따라다닐 NavMeshBoundsVolume (BeginPlay에서 레벨에서 찾아 캐시) */
	UPROPERTY()
	TObjectPtr<ANavMeshBoundsVolume> NavBoundsVolume;

	float TimeSinceUpdate = 0.f;
	bool bHasGenerated = false;
	FIntPoint LastPlayerChunk = FIntPoint(MAX_int32, MAX_int32);

	/** NavMeshBoundsVolume를 플레이어 위치로 옮기고 내비 시스템에 갱신을 통지 */
	void UpdateNavBoundsToPlayer();

	FIntPoint WorldToChunk(const FVector& WorldLocation) const;
	void UpdateChunks(const FIntPoint& Center);
	void GenerateChunk(const FIntPoint& Coord);
	void UnloadChunk(const FIntPoint& Coord);

	AStaticMeshActor* SpawnMeshActor(UStaticMesh* Mesh, const FVector& Location,
		const FRotator& Rotation, const FVector& Scale, UMaterialInterface* OverrideMat);
};
