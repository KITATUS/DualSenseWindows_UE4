// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Templates/SharedPointer.h"
#include "IDS5W_UE4.h"
#include "DS5WInterface.h"

#define LOCTEXT_NAMESPACE "DualSenseModule"

static FName ModuleName = FName("DualSense");

// Setup gyroscope
const FKey FDS5WKey::DS5W_GyroAxis_X("DS5W_GyroAxis_X");
const FKey FDS5WKey::DS5W_GyroAxis_Y("DS5W_GyroAxis_Y");

// Setup gyroscope names
const FDS5WKeyNames::Type FDS5WKeyNames::DS5W_GyroAxis_X("DS5W_GyroAxis_X");
const FDS5WKeyNames::Type FDS5WKeyNames::DS5W_GyroAxis_Y("DS5W_GyroAxis_Y");

class FDS5W_UE4 : public IDS5W_UE4
{
    /** Implements the rest of the IInputDeviceModule interface **/

    /** Creates a new instance of the IInputDevice associated with this IInputDeviceModule **/
    virtual TSharedPtr< class IInputDevice > CreateInputDevice(const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler);

    /** Called right after the module DLL has been loaded and the module object has been created **/
    virtual void StartupModule() override;

    /** Called before the module is unloaded, right before the module object is destroyed. **/
    virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE( FDS5W_UE4, DS5W_UE4 )

TSharedPtr< class IInputDevice > FDS5W_UE4::CreateInputDevice(const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler)
{
    UE_LOG(LogTemp, Warning, TEXT("Created new input device!"));
    return MakeShareable(new FDS5WInterface(InMessageHandler));
}

void FDS5W_UE4::StartupModule()
{
    // This code will execute after your module is loaded into memory (but after global variables are initialized, of course.)
    // Custom module-specific init can go here.
		
		// Register the FKeys
		EKeys::AddMenuCategoryDisplayInfo(ModuleName, LOCTEXT("DualSenseSubCategory", "DualSense"), TEXT("GraphEditor.PadEvent_16x"));

		// Gyroscope
		EKeys::AddKey(FKeyDetails(FDS5WKey::DS5W_GyroAxis_X, LOCTEXT("DS5W_GyroAxis_X", "DualSense Gyroscope X"), FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, ModuleName));
		EKeys::AddKey(FKeyDetails(FDS5WKey::DS5W_GyroAxis_Y, LOCTEXT("DS5W_GyroAxis_Y", "DualSense Gyroscope Y"), FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, ModuleName));

    UE_LOG(LogTemp, Warning, TEXT("DS5W_UE4 initiated!"));

    // IMPORTANT: This line registers our input device module with the engine.
    //        If we do not register the input device module with the engine,
    //        the engine won't know about our existence. Which means 
    //        CreateInputDevice never gets called, which means the engine
    //        will never try to poll for events from our custom input device.
    IModularFeatures::Get().RegisterModularFeature(IInputDeviceModule::GetModularFeatureName(), this);
}

void FDS5W_UE4::ShutdownModule()
{
    // This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
    // we call this function before unloading the module.

    UE_LOG(LogTemp, Warning, TEXT("DS5W_UE4 shut down!"));

    // Unregister our input device module
    IModularFeatures::Get().UnregisterModularFeature(IInputDeviceModule::GetModularFeatureName(), this);
}