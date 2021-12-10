#pragma once

#include "Modules/ModuleManager.h"
#include "IInputDeviceModule.h"
#include "InputCoreTypes.h"

struct FDS5WKey {
	/* Gyroscope */
	static const FKey DS5W_GyroAxis_X;
	static const FKey DS5W_GyroAxis_Y;
};

struct FDS5WKeyNames {
	typedef FName Type;

	/* Gyroscope axises */
	static const FName DS5W_GyroAxis_X;
	static const FName DS5W_GyroAxis_Y;
};

class IDS5W_UE4 : public IInputDeviceModule
{
public:

    static inline IDS5W_UE4& Get() 
    {
        return FModuleManager::LoadModuleChecked< IDS5W_UE4 >("DS5W_UE4");
    }

    static inline bool IsAvailable() 
    {
        return FModuleManager::Get().IsModuleLoaded("DS5W_UE4");
    }
};