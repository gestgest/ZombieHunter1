// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZombieHunter1GameMode.h"
#include "ZombieHunter1Character.h"
#include "UObject/ConstructorHelpers.h"

AZombieHunter1GameMode::AZombieHunter1GameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
