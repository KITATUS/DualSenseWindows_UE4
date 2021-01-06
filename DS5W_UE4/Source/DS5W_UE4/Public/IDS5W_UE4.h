#pragma once

#include "Modules/ModuleManager.h"
#include "IInputDeviceModule.h"
#include "InputCoreTypes.h"

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