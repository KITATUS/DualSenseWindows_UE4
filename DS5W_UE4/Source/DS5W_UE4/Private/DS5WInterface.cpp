// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "DS5WInterface.h"
#include "HAL/PlatformTime.h"
#include "Misc/CoreDelegates.h"
#include "Misc/ConfigCacheIni.h"
#include "..\Public\DS5WInterface.h"

#define DS5W_LEFT_THUMB_DEADZONE  30
#define DS5W_RIGHT_THUMB_DEADZONE 30
#define DS5W_TRIGGER_THRESHOLD    30

FDS5WInterface::FDS5WInterface(const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler) : MessageHandler(InMessageHandler)
{
	for (int32 ControllerIndex = 0; ControllerIndex < MAX_NUM_DS5W_CONTROLLERS; ++ControllerIndex)
	{
		FControllerState& ControllerState = ControllerStates[ControllerIndex];
		FMemory::Memzero(&ControllerState, sizeof(FControllerState));

		ControllerState.ControllerId = ControllerIndex;
	}

	bIsGamepadAttached = true;
	bNeedsControllerStateUpdate = true;
	InitialButtonRepeatDelay = 0.2f;
	ButtonRepeatDelay = 0.1f;

	GConfig->GetFloat(TEXT("/Script/Engine.InputSettings"), TEXT("InitialButtonRepeatDelay"), InitialButtonRepeatDelay, GInputIni);
	GConfig->GetFloat(TEXT("/Script/Engine.InputSettings"), TEXT("ButtonRepeatDelay"), ButtonRepeatDelay, GInputIni);

	// In the engine, all controllers map to xbox controllers for consistency 
	DS5WToXboxControllerMapping[0] = 0;		// A
	DS5WToXboxControllerMapping[1] = 1;		// B
	DS5WToXboxControllerMapping[2] = 2;		// X
	DS5WToXboxControllerMapping[3] = 3;		// Y
	DS5WToXboxControllerMapping[4] = 4;		// L1
	DS5WToXboxControllerMapping[5] = 5;		// R1
	DS5WToXboxControllerMapping[6] = 7;		// Back 
	DS5WToXboxControllerMapping[7] = 6;		// Start
	DS5WToXboxControllerMapping[8] = 8;		// Left thumbstick
	DS5WToXboxControllerMapping[9] = 9;		// Right thumbstick
	DS5WToXboxControllerMapping[10] = 10;	// L2
	DS5WToXboxControllerMapping[11] = 11;	// R2
	DS5WToXboxControllerMapping[12] = 12;	// Dpad up
	DS5WToXboxControllerMapping[13] = 13;	// Dpad down
	DS5WToXboxControllerMapping[14] = 14;	// Dpad left
	DS5WToXboxControllerMapping[15] = 15;	// Dpad right
	DS5WToXboxControllerMapping[16] = 16;	// Left stick up
	DS5WToXboxControllerMapping[17] = 17;	// Left stick down
	DS5WToXboxControllerMapping[18] = 18;	// Left stick left
	DS5WToXboxControllerMapping[19] = 19;	// Left stick right
	DS5WToXboxControllerMapping[20] = 20;	// Right stick up
	DS5WToXboxControllerMapping[21] = 21;	// Right stick down
	DS5WToXboxControllerMapping[22] = 22;	// Right stick left
	DS5WToXboxControllerMapping[23] = 23;	// Right stick right
	DS5WToXboxControllerMapping[24] = 24;	// Playstation Logo
	DS5WToXboxControllerMapping[25] = 25;	// Touchpad Button
	DS5WToXboxControllerMapping[26] = 26;	// Microphone Button

	Buttons[0] = FGamepadKeyNames::FaceButtonBottom;
	Buttons[1] = FGamepadKeyNames::FaceButtonRight;
	Buttons[2] = FGamepadKeyNames::FaceButtonLeft;
	Buttons[3] = FGamepadKeyNames::FaceButtonTop;
	Buttons[4] = FGamepadKeyNames::LeftShoulder;
	Buttons[5] = FGamepadKeyNames::RightShoulder;
	Buttons[6] = FGamepadKeyNames::SpecialRight;
	Buttons[7] = FGamepadKeyNames::SpecialLeft;
	Buttons[8] = FGamepadKeyNames::LeftThumb;
	Buttons[9] = FGamepadKeyNames::RightThumb;
	Buttons[10] = FGamepadKeyNames::LeftTriggerThreshold;
	Buttons[11] = FGamepadKeyNames::RightTriggerThreshold;
	Buttons[12] = FGamepadKeyNames::DPadUp;
	Buttons[13] = FGamepadKeyNames::DPadDown;
	Buttons[14] = FGamepadKeyNames::DPadLeft;
	Buttons[15] = FGamepadKeyNames::DPadRight;
	Buttons[16] = FGamepadKeyNames::LeftStickUp;
	Buttons[17] = FGamepadKeyNames::LeftStickDown;
	Buttons[18] = FGamepadKeyNames::LeftStickLeft;
	Buttons[19] = FGamepadKeyNames::LeftStickRight;
	Buttons[20] = FGamepadKeyNames::RightStickUp;
	Buttons[21] = FGamepadKeyNames::RightStickDown;
	Buttons[22] = FGamepadKeyNames::RightStickLeft;
	Buttons[23] = FGamepadKeyNames::RightStickRight;

	//TODO: These buttons aren't named in Gamepad keys. That should probably be fixed at some point.
	Buttons[24] = FGamepadKeyNames::Invalid;
	Buttons[25] = FGamepadKeyNames::Invalid;
	Buttons[26] = FGamepadKeyNames::Invalid;

	DS5W::DeviceEnumInfo infos[16];
	unsigned int controllersCount = 0;
	switch (DS5W::enumDevices(infos, 16, &controllersCount)) 
	{
		case DS5W_OK:
		case DS5W_E_INSUFFICIENT_BUFFER:
			UE_LOG(LogTemp, Error, TEXT("FDS5WInterface::FDS5WInterface: Insuffienct Buffer."));
			break;
		default:
		UE_LOG(LogTemp, Error, TEXT("FDS5WInterface::FDS5WInterface: Unknown Error. Aborting."));
		bIsGamepadAttached = false;
		return;
	}

	if (!controllersCount)
	{
		UE_LOG(LogTemp, Error, TEXT("FDS5WInterface::FDS5WInterface: Controller Count not valid! Aborting"));
		bIsGamepadAttached = false;		
		return;
	}

	if (controllersCount > MAX_NUM_DS5W_CONTROLLERS)
	{
		UE_LOG(LogTemp, Warning, TEXT("FDS5WInterface::FDS5WInterface: Found more DS5 controllers than we can account for. Not all of them will work as intended!"));
	}

	if (DS5W_FAILED(DS5W::initDeviceContext(&infos[0], &con))) 
	{
		UE_LOG(LogTemp, Error, TEXT("FDS5WInterface::FDS5WInterface: Failure initializing device. Aborting."));
		bIsGamepadAttached = false;
		return;
	}
}

FDS5WInterface::~FDS5WInterface()
{
	DS5W::freeDeviceContext(&con);
}

void FDS5WInterface::SendControllerEvents()
{
	DS5W::DS5InputState DS5WStates[MAX_NUM_DS5W_CONTROLLERS];
	DS5W::DS5OutputState DS5WOutputStates[MAX_NUM_DS5W_CONTROLLERS];
	bool bWereConnected[MAX_NUM_DS5W_CONTROLLERS];
	bIsGamepadAttached = false;
	int btMul = con._internal.connection == DS5W::DeviceConnection::BT ? 10 : 1;

	for (int32 ControllerIndex = 0; ControllerIndex < MAX_NUM_DS5W_CONTROLLERS; ++ControllerIndex)
	{
		FControllerState& ControllerState = ControllerStates[ControllerIndex];

		bWereConnected[ControllerIndex] = ControllerState.bIsConnected;

		if (ControllerState.bIsConnected || bNeedsControllerStateUpdate)
		{
			DS5W::DS5InputState& DS5WState = DS5WStates[ControllerIndex];
			DS5W::DS5OutputState& DS5WOutputState = DS5WOutputStates[ControllerIndex];
			FMemory::Memzero(&DS5WState, sizeof(DS5W::DS5InputState));

			ControllerState.bIsConnected = (DS5W_SUCCESS(DS5W::getDeviceInputState(&con, &DS5WState)));
			if (ControllerState.bIsConnected)
			{
				bIsGamepadAttached = true;
			}
		}
	}

	for (int32 ControllerIndex = 0; ControllerIndex < MAX_NUM_DS5W_CONTROLLERS; ++ControllerIndex)
	{
		// Set input scope, there doesn't seem to be a reliable way to differentiate 360 vs Xbox one controllers so use generic name
		FInputDeviceScope InputScope(this, DS5WInterfaceName, ControllerIndex, DS5WControllerIdentifier);

		FControllerState& ControllerState = ControllerStates[ControllerIndex];

		const bool bWasConnected = bWereConnected[ControllerIndex];

		// If the controller is connected send events or if the controller was connected send a final event with default states so that 
		// the game doesn't think that controller buttons are still held down
		if (ControllerState.bIsConnected || bWasConnected)
		{
			const DS5W::DS5InputState& DS5WState = DS5WStates[ControllerIndex];
			DS5W::DS5OutputState DS5WOutputState = DS5WOutputStates[ControllerIndex];

			// If the controller is connected now but was not before, refresh the information
			if (!bWasConnected && ControllerState.bIsConnected)
			{
				FCoreDelegates::OnControllerConnectionChange.Broadcast(true, -1, ControllerState.ControllerId);
			}
			else if (bWasConnected && !ControllerState.bIsConnected)
			{
				FCoreDelegates::OnControllerConnectionChange.Broadcast(false, -1, ControllerState.ControllerId);
			}

			bool CurrentStates[MAX_NUM_CONTROLLER_BUTTONS] = { 0 };

			// Get the current state of all buttons
			CurrentStates[DS5WToXboxControllerMapping[0]] = !!(DS5WState.buttonsAndDpad & DS5W_ISTATE_BTX_CROSS);
			CurrentStates[DS5WToXboxControllerMapping[1]] = !!(DS5WState.buttonsAndDpad & DS5W_ISTATE_BTX_CIRCLE);
			CurrentStates[DS5WToXboxControllerMapping[2]] = !!(DS5WState.buttonsAndDpad & DS5W_ISTATE_BTX_SQUARE);
			CurrentStates[DS5WToXboxControllerMapping[3]] = !!(DS5WState.buttonsAndDpad & DS5W_ISTATE_BTX_TRIANGLE);
			CurrentStates[DS5WToXboxControllerMapping[4]] = !!(DS5WState.buttonsA & DS5W_ISTATE_BTN_A_LEFT_BUMPER);
			CurrentStates[DS5WToXboxControllerMapping[5]] = !!(DS5WState.buttonsA & DS5W_ISTATE_BTN_A_RIGHT_BUMPER);
			CurrentStates[DS5WToXboxControllerMapping[6]] = !!(DS5WState.buttonsA & DS5W_ISTATE_BTN_A_SELECT);
			CurrentStates[DS5WToXboxControllerMapping[7]] = !!(DS5WState.buttonsA & DS5W_ISTATE_BTN_A_MENU);
			CurrentStates[DS5WToXboxControllerMapping[8]] = !!(DS5WState.buttonsA & DS5W_ISTATE_BTN_A_LEFT_STICK);
			CurrentStates[DS5WToXboxControllerMapping[9]] = !!(DS5WState.buttonsA & DS5W_ISTATE_BTN_A_RIGHT_STICK);
			CurrentStates[DS5WToXboxControllerMapping[10]] = !!(DS5WState.leftTrigger > DS5W_ISTATE_BTN_A_LEFT_TRIGGER);
			CurrentStates[DS5WToXboxControllerMapping[11]] = !!(DS5WState.rightTrigger > DS5W_ISTATE_BTN_A_RIGHT_TRIGGER);
			CurrentStates[DS5WToXboxControllerMapping[12]] = !!(DS5WState.buttonsAndDpad & DS5W_ISTATE_DPAD_UP);
			CurrentStates[DS5WToXboxControllerMapping[13]] = !!(DS5WState.buttonsAndDpad & DS5W_ISTATE_DPAD_DOWN);
			CurrentStates[DS5WToXboxControllerMapping[14]] = !!(DS5WState.buttonsAndDpad & DS5W_ISTATE_DPAD_LEFT);
			CurrentStates[DS5WToXboxControllerMapping[15]] = !!(DS5WState.buttonsAndDpad & DS5W_ISTATE_DPAD_RIGHT);
			CurrentStates[DS5WToXboxControllerMapping[16]] = !!(DS5WState.leftStick.y > DS5W_LEFT_THUMB_DEADZONE);
			CurrentStates[DS5WToXboxControllerMapping[17]] = !!(DS5WState.leftStick.y < -DS5W_LEFT_THUMB_DEADZONE);
			CurrentStates[DS5WToXboxControllerMapping[18]] = !!(DS5WState.leftStick.x < -DS5W_LEFT_THUMB_DEADZONE);
			CurrentStates[DS5WToXboxControllerMapping[19]] = !!(DS5WState.leftStick.x > DS5W_LEFT_THUMB_DEADZONE);
			CurrentStates[DS5WToXboxControllerMapping[20]] = !!(DS5WState.rightStick.y > DS5W_RIGHT_THUMB_DEADZONE);
			CurrentStates[DS5WToXboxControllerMapping[21]] = !!(DS5WState.rightStick.y < -DS5W_RIGHT_THUMB_DEADZONE);
			CurrentStates[DS5WToXboxControllerMapping[22]] = !!(DS5WState.rightStick.y < -DS5W_RIGHT_THUMB_DEADZONE);
			CurrentStates[DS5WToXboxControllerMapping[23]] = !!(DS5WState.rightStick.y > DS5W_RIGHT_THUMB_DEADZONE);
			CurrentStates[DS5WToXboxControllerMapping[24]] = !!(DS5WState.buttonsB & DS5W_ISTATE_BTN_B_PLAYSTATION_LOGO);
			CurrentStates[DS5WToXboxControllerMapping[25]] = !!(DS5WState.buttonsB & DS5W_ISTATE_BTN_B_PAD_BUTTON);
			CurrentStates[DS5WToXboxControllerMapping[26]] = !!(DS5WState.buttonsB & DS5W_ISTATE_BTN_B_MIC_BUTTON);
			
			//TODO: Add touchpad

			// Send new analog data if it's different or outside the platform deadzone.
			auto OnControllerAnalog = [this, &ControllerState](const FName& GamePadKey, const auto NewAxisValue, const float NewAxisValueNormalized, auto& OldAxisValue, const auto DeadZone) 
			{
				if (OldAxisValue != NewAxisValue || FMath::Abs((int32)NewAxisValue) > DeadZone)
				{
					MessageHandler->OnControllerAnalog(GamePadKey, ControllerState.ControllerId, NewAxisValueNormalized);
				}

				OldAxisValue = NewAxisValue;
			};

			const auto& Gamepad = DS5WState;

			OnControllerAnalog(FGamepadKeyNames::LeftAnalogX, Gamepad.leftStick.x, FMath::GetMappedRangeValueClamped(FVector2D(-127, 127), FVector2D(-1, 1), Gamepad.leftStick.x), ControllerState.LeftXAnalog, DS5W_LEFT_THUMB_DEADZONE);
			OnControllerAnalog(FGamepadKeyNames::LeftAnalogY, Gamepad.leftStick.y, FMath::GetMappedRangeValueClamped(FVector2D(-127, 127), FVector2D(-1, 1), Gamepad.leftStick.y), ControllerState.LeftYAnalog, DS5W_LEFT_THUMB_DEADZONE);
			OnControllerAnalog(FGamepadKeyNames::RightAnalogX, Gamepad.rightStick.x, FMath::GetMappedRangeValueClamped(FVector2D(-127, 127), FVector2D(-1, 1), Gamepad.rightStick.x), ControllerState.RightXAnalog, DS5W_RIGHT_THUMB_DEADZONE);
			OnControllerAnalog(FGamepadKeyNames::RightAnalogY, Gamepad.rightStick.y, FMath::GetMappedRangeValueClamped(FVector2D(-127, 127), FVector2D(-1, 1), Gamepad.rightStick.y), ControllerState.RightYAnalog, DS5W_RIGHT_THUMB_DEADZONE);

			ControllerState.LeftXAnalog = Gamepad.leftStick.x;
			ControllerState.LeftYAnalog = Gamepad.leftStick.y;
			ControllerState.RightXAnalog = Gamepad.rightStick.x;
			ControllerState.RightYAnalog = Gamepad.rightStick.y;

			OnControllerAnalog(FGamepadKeyNames::LeftTriggerAnalog, Gamepad.leftTrigger, Gamepad.leftTrigger / 255.f, ControllerState.LeftTriggerAnalog, DS5W_TRIGGER_THRESHOLD);
			OnControllerAnalog(FGamepadKeyNames::RightTriggerAnalog, Gamepad.rightTrigger, Gamepad.rightTrigger / 255.f, ControllerState.RightTriggerAnalog, DS5W_TRIGGER_THRESHOLD);

			const double CurrentTime = FPlatformTime::Seconds();

			// For each button check against the previous state and send the correct message if any
			for (int32 ButtonIndex = 0; ButtonIndex < MAX_NUM_CONTROLLER_BUTTONS; ++ButtonIndex)
			{
				if (CurrentStates[ButtonIndex] != ControllerState.ButtonStates[ButtonIndex])
				{
					if (CurrentStates[ButtonIndex])
					{
						MessageHandler->OnControllerButtonPressed(Buttons[ButtonIndex], ControllerState.ControllerId, false);
					}
					else
					{
						MessageHandler->OnControllerButtonReleased(Buttons[ButtonIndex], ControllerState.ControllerId, false);
					}

					if (CurrentStates[ButtonIndex] != 0)
					{
						// this button was pressed - set the button's NextRepeatTime to the InitialButtonRepeatDelay
						ControllerState.NextRepeatTime[ButtonIndex] = CurrentTime + InitialButtonRepeatDelay;
					}
				}
				else if (CurrentStates[ButtonIndex] != 0 && ControllerState.NextRepeatTime[ButtonIndex] <= CurrentTime)
				{
					MessageHandler->OnControllerButtonPressed(Buttons[ButtonIndex], ControllerState.ControllerId, true);

					// set the button's NextRepeatTime to the ButtonRepeatDelay
					ControllerState.NextRepeatTime[ButtonIndex] = CurrentTime + ButtonRepeatDelay;
				}

				// Update the state for next time
				ControllerState.ButtonStates[ButtonIndex] = CurrentStates[ButtonIndex];
			}

			// apply force feedback

			const float LargeValue = (ControllerState.ForceFeedback.LeftLarge > ControllerState.ForceFeedback.RightLarge ? ControllerState.ForceFeedback.LeftLarge : ControllerState.ForceFeedback.RightLarge);
			const float SmallValue = (ControllerState.ForceFeedback.LeftSmall > ControllerState.ForceFeedback.RightSmall ? ControllerState.ForceFeedback.LeftSmall : ControllerState.ForceFeedback.RightSmall);

			// Player led
			DS5WOutputState.playerLeds.playerLedFade = ControllerState.LEDState.Fade;
			DS5WOutputState.playerLeds.bitmask = LEDIntToBitMask(ControllerState.LEDState);
			DS5WOutputState.playerLeds.brightness = LEDBrightnessToEnum(ControllerState.LEDState);

			// Lightbar
			DS5WOutputState.lightbar = DS5W::color_R8G8B8_UCHAR_A32_FLOAT((int)ControllerState.LightBarState.X, (int)ControllerState.LightBarState.Y, (int)ControllerState.LightBarState.Z, (int)ControllerState.LightBarState.W);

			DS5WOutputState.leftRumble = 0;
			DS5WOutputState.rightRumble = 0;

			DS5W::setDeviceOutputState(&con, &DS5WOutputState);

		}
	}

	bNeedsControllerStateUpdate = false;

}

void FDS5WInterface::SetMessageHandler(const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler)
{
    MessageHandler = InMessageHandler;
}

void FDS5WInterface::SetChannelValue(int32 ControllerId, const FForceFeedbackChannelType ChannelType, const float Value)
{
	if (ControllerId >= 0 && ControllerId < MAX_NUM_DS5W_CONTROLLERS)
	{
		FControllerState& ControllerState = ControllerStates[ControllerId];

		if (ControllerState.bIsConnected)
		{
			switch (ChannelType)
			{
			case FForceFeedbackChannelType::LEFT_LARGE:
				ControllerState.ForceFeedback.LeftLarge = Value;
				break;

			case FForceFeedbackChannelType::LEFT_SMALL:
				ControllerState.ForceFeedback.LeftSmall = Value;
				break;

			case FForceFeedbackChannelType::RIGHT_LARGE:
				ControllerState.ForceFeedback.RightLarge = Value;
				break;

			case FForceFeedbackChannelType::RIGHT_SMALL:
				ControllerState.ForceFeedback.RightSmall = Value;
				break;
			}
		}
	}
}

void FDS5WInterface::SetChannelValues(int32 ControllerId, const FForceFeedbackValues& Values)
{
	if (ControllerId >= 0 && ControllerId < MAX_NUM_DS5W_CONTROLLERS)
	{
		FControllerState& ControllerState = ControllerStates[ControllerId];

		if (ControllerState.bIsConnected)
		{
			ControllerState.ForceFeedback = Values;
		}
	}
}

int FDS5WInterface::LEDIntToBitMask(FPlayerLED LEDRef)
{
	switch (LEDRef.PlayerLEDState)
	{
	case 1:
		return 0x01;
	case 2:
		return 0x02;
	case 3:
		return 0x04;
	case 4:
		return 0x08;
	case 5:
		return 0x10;
	default:
		return 0;
	}
}

DS5W::LedBrightness FDS5WInterface::LEDBrightnessToEnum(FPlayerLED LEDRef)
{
	switch (LEDRef.Brightness)
	{
	case 1:
		return DS5W::LedBrightness::HIGH;
	case 2:
		return DS5W::LedBrightness::MEDIUM;
	default:
		return DS5W::LedBrightness::LOW;
	}
}
