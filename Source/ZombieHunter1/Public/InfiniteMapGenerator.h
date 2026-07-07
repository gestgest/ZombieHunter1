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

/** POI(특수 지역) 종류 */
enum class EPOIType : uint8
{
	Village,		// 마을 (발판/NPC 등 — 후속 단계에서 채움)
	ZombieVillage,	// 좀비마을 (적 밀집 + 보상 — 후속 단계에서 채움)
};

/** 리전 하나의 POI 정보. 시드 해시로만 결정되므로 어느 청크에서 계산해도 항상 같은 답이 나온다. */
struct FPOIInfo
{
	bool bHasPOI = false;
	FIntPoint CenterChunk = FIntPoint::ZeroValue;	// POI 중심 청크 좌표 (리전 로컬 아님, 전역 청크 좌표)
	EPOIType Type = EPOIType::Village;
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

	//7 7?
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

	//////////////////////////////////////////////////////////////////////////
	// POI (마을/좀비마을 — 특수 지역)
	// 청크를 리전(RegionSizeInChunks²) 단위로 묶고, 리전마다 시드 해시로 최대 1개의 POI 청크를 정한다.
	// POI 청크는 장애물을 생성하지 않고 전용 바닥을 깐다. (스켈레톤 — 콘텐츠는 후속 단계)

	//그러니까 8x8청크에 하나의 리전 존재
	/** 리전 한 변의 청크 수. 리전마다 최대 1개의 POI가 배치된다. (8 × ChunkSize 2000 = 160m마다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|POI", meta = (ClampMin = "2"))
	int32 RegionSizeInChunks = 8;

	/** 리전에 POI가 생길 확률(0~1). 시작 리전(0,0)은 이 값과 무관하게 항상 마을이 보장된다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|POI", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float POIChance = 0.8f;

	/** POI가 마을일 확률(나머지는 좀비마을). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|POI", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float VillageRatio = 0.6f;

	/** 마을 청크의 바닥 머티리얼. 기본값: MI_Solid_Blue (파란 바닥 = 마을) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|POI")
	TObjectPtr<UMaterialInterface> VillageFloorMaterial;

	/** 좀비마을 청크의 바닥 머티리얼. 기본값: MI_PrototypeGrid_TopDark (어두운 바닥 = 좀비마을) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|POI")
	TObjectPtr<UMaterialInterface> ZombieVillageFloorMaterial;

	/** 켜면 POI 청크 생성 시 경계 박스(마을=초록, 좀비마을=빨강)와 로그를 남긴다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|POI|Debug")
	bool bDebugDrawPOI = true;

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

	/** 청크 좌표 → 소속 리전 좌표 (음수 좌표도 올바르게 내림 나눗셈) */
	FIntPoint ChunkToRegion(const FIntPoint& ChunkCoord) const;

	/** 이 리전의 POI 정보를 시드 해시로 계산한다. 스폰/검사 없음 — 순수 계산이라 항상 같은 답. */
	FPOIInfo GetPOIForRegion(const FIntPoint& RegionCoord) const;

	/** 이 청크가 POI 중심 청크면 true를 반환하고 OutInfo를 채운다. */
	bool GetPOIAtChunk(const FIntPoint& ChunkCoord, FPOIInfo& OutInfo) const;

	AStaticMeshActor* SpawnMeshActor(UStaticMesh* Mesh, const FVector& Location,
		const FRotator& Rotation, const FVector& Scale, UMaterialInterface* OverrideMat);
};
