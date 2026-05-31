// Fill out your copyright notice in the Description page of Project Settings.

#include "InfiniteMapGenerator.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"


//초기
AInfiniteMapGenerator::AInfiniteMapGenerator()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));



	// 기본 메시 자동 지정 (엔진 기본 도형은 항상 존재하고 한 변이 정확히 100cm).
	// 바닥을 생성하고 나서 위의 오브젝트들
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded())
	{
		FloorMesh = CubeFinder.Object;
		ObstacleMeshes.Add(CubeFinder.Object);
	}
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderFinder.Succeeded())
	{
		ObstacleMeshes.Add(CylinderFinder.Object);
	}
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeFinder(TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (ConeFinder.Succeeded())
	{
		ObstacleMeshes.Add(ConeFinder.Object);
	}


	// 바닥 머티리얼(있으면): 프로젝트의 프로토타입 그리드
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GridMatFinder(TEXT("/Game/LevelPrototyping/Materials/MI_PrototypeGrid_Gray.MI_PrototypeGrid_Gray"));
	if (GridMatFinder.Succeeded())
	{
		FloorMaterial = GridMatFinder.Object;
	}
}

void AInfiniteMapGenerator::BeginPlay()
{
	Super::BeginPlay();

	//플레이어를 가져와라
	TrackedPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (TrackedPawn)
	{
		const FIntPoint Center = WorldToChunk(TrackedPawn->GetActorLocation()); //벡터값 가져오기
		UpdateChunks(Center);
		LastPlayerChunk = Center;
		bHasGenerated = true;
	}
}



void AInfiniteMapGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TimeSinceUpdate += DeltaTime;
	if (TimeSinceUpdate < UpdateInterval)
	{
		return;
	}
	TimeSinceUpdate = 0.f;

	//BeginPlay보다 빠르게 Tick 함수가 호출되는 경우 대비
	if (!TrackedPawn)
	{
		TrackedPawn = UGameplayStatics::GetPlayerPawn(this, 0);
		if (!TrackedPawn)
		{
			return;
		}
	}

	const FIntPoint Center = WorldToChunk(TrackedPawn->GetActorLocation());
	if (!bHasGenerated || Center != LastPlayerChunk) //전 청크와 내 청크가 다르다면
	{
		UpdateChunks(Center);
		LastPlayerChunk = Center;
		bHasGenerated = true;
	}
}

//내 위치의 청크 반환
FIntPoint AInfiniteMapGenerator::WorldToChunk(const FVector& WorldLocation) const
{
	const float Size = FMath::Max(1.f, ChunkSize);
	return FIntPoint(FMath::FloorToInt(WorldLocation.X / Size),
		FMath::FloorToInt(WorldLocation.Y / Size));
}

void AInfiniteMapGenerator::UpdateChunks(const FIntPoint& Center)
{
	// 1) 필요한 청크 집합 계산 7x7 
	TSet<FIntPoint> needed;
	needed.Reserve((2 * ViewRadiusInChunks + 1) * (2 * ViewRadiusInChunks + 1));
	for (int32 dx = -ViewRadiusInChunks; dx <= ViewRadiusInChunks; ++dx)
	{
		for (int32 dy = -ViewRadiusInChunks; dy <= ViewRadiusInChunks; ++dy)
		{
			needed.Add(FIntPoint(Center.X + dx, Center.Y + dy));
		}
	}

	// 2) 범위를 벗어난 청크 언로드
	TArray<FIntPoint> ToUnload;
	for (const TPair<FIntPoint, FMapChunk>& Pair : LoadedChunks)
	{
		if (!needed.Contains(Pair.Key))
		{
			ToUnload.Add(Pair.Key);
		}
	}
	for (const FIntPoint& Coord : ToUnload)
	{
		UnloadChunk(Coord);
	}

	// 3) 새로 들어온 청크 생성
	for (const FIntPoint& Coord : needed)
	{
		if (!LoadedChunks.Contains(Coord))
		{
			GenerateChunk(Coord);
		}
	}
}

//언로드. Destroy로 제거한다.
void AInfiniteMapGenerator::UnloadChunk(const FIntPoint& Coord)
{
	if (FMapChunk* Chunk = LoadedChunks.Find(Coord))
	{
		for (AActor* Actor : Chunk->SpawnedActors)
		{
			if (IsValid(Actor))
			{
				Actor->Destroy();
			}
		}
		LoadedChunks.Remove(Coord);
	}
}

void AInfiniteMapGenerator::GenerateChunk(const FIntPoint& Coord)
{
	FMapChunk Chunk;

	const FVector Origin(Coord.X * ChunkSize, Coord.Y * ChunkSize, 0.f);
	const FVector Center = Origin + FVector(ChunkSize * 0.5f, ChunkSize * 0.5f, 0.f);

	// 청크 좌표 + 전역 시드로 결정론적 난수 (같은 청크는 항상 동일하게 재생성)
	const uint32 Hash = HashCombine(GetTypeHash(Coord), static_cast<uint32>(GlobalSeed));
	FRandomStream Stream(static_cast<int32>(Hash));

	// 바닥: 청크 전체를 덮도록 스케일, 윗면이 Z=0에 오도록 배치
	if (FloorMesh)
	{
		const float Base = FMath::Max(1.f, FloorMeshBaseSize);
		const float XYScale = ChunkSize / Base;
		const float ZScale = FloorThickness / Base;
		const FVector FloorScale(XYScale, XYScale, ZScale);
		const FVector FloorLoc = Center - FVector(0.f, 0.f, FloorThickness * 0.5f);

		if (AStaticMeshActor* Floor = SpawnMeshActor(FloorMesh, FloorLoc, FRotator::ZeroRotator, FloorScale, FloorMaterial))
		{
			Chunk.SpawnedActors.Add(Floor);
		}
	}

	// 장애물: 청크 안에 랜덤 배치 (중앙 일부는 비워두고 싶으면 ChunkEdgeMargin 조정)
	if (ObstacleMeshes.Num() > 0)
	{
		const int32 Count = Stream.RandRange(MinObstaclesPerChunk, FMath::Max(MinObstaclesPerChunk, MaxObstaclesPerChunk));
		for (int32 i = 0; i < Count; ++i)
		{
			UStaticMesh* Mesh = ObstacleMeshes[Stream.RandRange(0, ObstacleMeshes.Num() - 1)];
			if (!Mesh)
			{
				continue;
			}

			const float LocalX = Stream.FRandRange(ChunkEdgeMargin, ChunkSize - ChunkEdgeMargin);
			const float LocalY = Stream.FRandRange(ChunkEdgeMargin, ChunkSize - ChunkEdgeMargin);
			const float Scale = Stream.FRandRange(ObstacleMinScale, ObstacleMaxScale);
			const float Yaw = Stream.FRandRange(0.f, 360.f);

			// 중심 피벗 메시를 바닥 위에 앉히기 위한 Z 오프셋
			const float HalfHeight = (ObstacleMeshBaseSize * Scale) * 0.5f;
			const FVector Loc = Origin + FVector(LocalX, LocalY, HalfHeight);

			if (AStaticMeshActor* Obstacle = SpawnMeshActor(Mesh, Loc, FRotator(0.f, Yaw, 0.f), FVector(Scale), nullptr))
			{
				Chunk.SpawnedActors.Add(Obstacle);
			}
		}
	}

	LoadedChunks.Add(Coord, MoveTemp(Chunk));
}

//오브젝트 배치라고 생각하면 됨
AStaticMeshActor* AInfiniteMapGenerator::SpawnMeshActor(UStaticMesh* Mesh, const FVector& Location,
	const FRotator& Rotation, const FVector& Scale, UMaterialInterface* OverrideMat)
{
	if (!Mesh || !GetWorld())
	{
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.Owner = this;

	AStaticMeshActor* Actor = GetWorld()->SpawnActor<AStaticMeshActor>(
		AStaticMeshActor::StaticClass(), Location, Rotation, Params);
	if (!Actor)
	{
		return nullptr;
	}

	if (UStaticMeshComponent* Comp = Actor->GetStaticMeshComponent())
	{
		// 런타임에 변형/메시 설정을 하려면 Movable 이어야 함 (기본은 Static)
		Comp->SetMobility(EComponentMobility::Movable);
		Actor->SetActorScale3D(Scale);
		Comp->SetStaticMesh(Mesh);
		if (OverrideMat)
		{
			Comp->SetMaterial(0, OverrideMat);
		}
		Comp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Comp->SetCollisionProfileName(TEXT("BlockAll"));
	}

	return Actor;
}
