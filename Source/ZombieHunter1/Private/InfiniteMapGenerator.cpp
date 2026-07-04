// Fill out your copyright notice in the Description page of Project Settings.

#include "InfiniteMapGenerator.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "NavigationSystem.h" // 런타임 스폰한 바닥을 NavMesh에 반영
#include "NavMesh/NavMeshBoundsVolume.h" // 플레이어 따라 옮길 나비 경계 볼륨


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

	// [디버그] 켜져 있으면 초기 생성도 안 함 (맵 없이 시작해 핑 비교용)
	if (debugDisableGeneration)
	{
		return;
	}

	//플레이어를 가져와라
	TrackedPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (TrackedPawn)
	{
		const FIntPoint Center = WorldToChunk(TrackedPawn->GetActorLocation()); //벡터값 가져오기
		UpdateChunks(Center);
		LastPlayerChunk = Center;
		bHasGenerated = true;
	}

	//플레이어 추격
	// NavMeshBoundsVolume를 레벨에서 찾아 캐시하고, 플레이어 위치로 한 번 맞춰둔다.
	if (bFollowNavBounds)
	{
		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsOfClass(this, ANavMeshBoundsVolume::StaticClass(), Found);
		if (Found.Num() > 0)
		{
			NavBoundsVolume = Cast<ANavMeshBoundsVolume>(Found[0]);

			// 런타임에 SetActorLocation으로 옮기려면 브러시(루트) 컴포넌트가 Movable이어야 한다.
			// (기본은 Static이라 "무버블이어야 합니다" 에러가 나고 이동이 무시됨)
			if (NavBoundsVolume && NavBoundsVolume->GetRootComponent())
			{
				NavBoundsVolume->GetRootComponent()->SetMobility(EComponentMobility::Movable);
			}
		}
		UpdateNavBoundsToPlayer();
	}
}



void AInfiniteMapGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// [디버그] 맵 생성이 핑 원인인지 격리: 켜면 청크 갱신을 통째로 건너뜀
	if (debugDisableGeneration)
	{
		return;
	}

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

		// 청크가 바뀐 시점에만(=플레이어가 한 청크 이동) NavMesh 경계도 플레이어로 따라 옮긴다.
		// 매 프레임이 아니라 이 시점에만 해서 내비 리빌드 부담을 줄인다.
		if (bFollowNavBounds)
		{
			UpdateNavBoundsToPlayer();
		}
	}
}

//플레이어를 추격하는 nav mesh
void AInfiniteMapGenerator::UpdateNavBoundsToPlayer()
{
	if (!NavBoundsVolume || !TrackedPawn)
	{
		return;
	}

	// 볼륨을 플레이어 XY로 옮긴다(높이는 유지 — 점프해도 경계가 위아래로 안 흔들리게).
	FVector Loc = TrackedPawn->GetActorLocation();
	Loc.Z = NavBoundsVolume->GetActorLocation().Z;
	NavBoundsVolume->SetActorLocation(Loc);

	// 핵심: 위치만 바꾸면 NavMesh가 새 위치에 안 깔린다. 내비 시스템에 "경계 바뀜"을 통지해야
	// 옮긴 영역에 NavMesh가 (인보커 기준으로) 다시 생성된다.
	if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		NavSys->OnNavigationBoundsUpdated(NavBoundsVolume);
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

#if WITH_EDITOR
	// 아웃라이너 정리용 폴더 (에디터/PIE 전용 — 패키징 빌드에선 컴파일 제외)
	Actor->SetFolderPath(TEXT("Spawned/Map"));
#endif

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

		// NavMesh 반영 — 런타임 스폰 시 컴포넌트는 "메시 없는 상태"로 내비에 등록되므로,
		// 메시/충돌을 다 세팅한 뒤 내비 관련성을 켜고 옥트리를 갱신해 줘야 이 바닥 위에 NavMesh가 깔린다.
		// (이게 없으면 동적 NavMesh가 런타임 바닥을 못 잡아서 적/동료가 길찾기를 못 함)
		Comp->SetCanEverAffectNavigation(true);
		if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
		{
			NavSys->UpdateComponentInNavOctree(*Comp);
		}
	}

	return Actor;
}
