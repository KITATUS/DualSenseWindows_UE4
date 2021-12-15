// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "GenericPlatform/IInputInterface.h"
#include "IInputDevice.h"
#include "GenericPlatform/GenericApplicationMessageHandler.h"

#include "DualSenseWindows/Device.h"
#include "DualSenseWindows/DS5State.h"
#include "DualSenseWindows/DSW_Api.h"
#include "DualSenseWindows/Helpers.h"
#include "DualSenseWindows/IO.h"

#include "GamepadMotion.hpp"

/** Max number of controllers. */
#define MAX_NUM_DS5W_CONTROLLERS 4

/** Max number of controller buttons.  Must be < 256*/
#define MAX_NUM_CONTROLLER_BUTTONS 27

enum class FForceFeedbackChannelType;

class FDS5WInterface : public IInputDevice
{
public:

    FDS5WInterface(const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler);
    ~FDS5WInterface();

    /** Tick the interface (e.g. check for new controllers) */
    virtual void Tick(float DeltaTime) override {};

    /** Poll for controller state and send events if needed */
    virtual void SendControllerEvents() override;

    /** Set which MessageHandler will get the events from SendControllerEvents. */
    virtual void SetMessageHandler(const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler) override;

    /** Exec handler to allow console commands to be passed through for debugging */
    virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override { return false; } ;

    void SetNeedsControllerStateUpdate() { bNeedsControllerStateUpdate = true; }
    virtual bool IsGamepadAttached() const override { return bIsGamepadAttached; }

    /** IForceFeedbackSystem pass through functions **/
	virtual void SetChannelValue(int32 ControllerId, const FForceFeedbackChannelType ChannelType, const float Value) override;
	virtual void SetChannelValues(int32 ControllerId, const FForceFeedbackValues& Values) override;

private:

	struct FPlayerLED
	{
		bool Fade = false;
		uint8 PlayerLEDState = 0; // Maps to PLAYER_LED, Left = 1, MiddleLeft = 2, Middle = 3, MiddleRight = 4, Right = 5
		uint8 Brightness = 0; //Maps to DS5W:LedBrightness. 0 = Low, 1 = Medium, 2 = High
	};

	struct FGyroscopeSensor {
	public:
		void Init(int ID);
		void Update(FVector Angle);
		void Reset();

		inline FVector2D& GetLastDelta() { return (LastDelta); }

	private:
		float SmoothingThreshold = 100.f;
		float TighteningThreshold = 100.f;

		int Id;

		double LastTime;
		FVector2D LastAngle;
		FVector2D LastDelta;
		FVector LastGyroValue;

		FVector GravityVector;

		template <typename T> inline constexpr
			int signum(T x, std::false_type is_signed) {
			return T(0) < x;
		}

		template <typename T> inline constexpr
			int signum(T x, std::true_type is_signed) {
			return (T(0) < x) - (x < T(0));
		}

		template <typename T> inline constexpr
			int signum(T x) {
			return signum(x, std::is_signed<T>());
		}

		FVector2D GyroCameraLocal(FVector gyro);
		FVector2D GyroCameraWorld(FVector gyro, FVector gravNorm);

		FVector2D GetDirectInput(FVector2D input) {
			return input;
		}

		// smoothing buffer
		TArray<FVector2D>::SizeType InputBufferSize;
		TArray<FVector2D> InputBuffer;
		int CurrentInputIndex;

		FVector2D GetSmoothedInput(FVector2D input) {
			CurrentInputIndex = (CurrentInputIndex + 1) % InputBuffer.Num();
			InputBuffer[CurrentInputIndex] = input;

			FVector2D average = FVector2D::ZeroVector;
			for (FVector2D sample : InputBuffer) {
				average += sample;
			}
			average /= InputBuffer.Num();

			return average;
		}

		FVector2D GetTieredSmoothedInput(FVector2D input,
			float threshold1, float threshold2) {

			float inputMagnitude = FMath::Sqrt(input.X * input.X +
				input.Y * input.Y);

			float directWeight = (inputMagnitude - threshold1) /
				(threshold2 - threshold1);
			directWeight = FMath::Clamp(directWeight, 0.f, 1.f);

			return GetDirectInput(input * directWeight) +
				GetSmoothedInput(input * (1.0 - directWeight));

		}

		FVector2D GetTightenedInput(FVector2D input, float threshold) {
			float inputMagnitude = FMath::Sqrt(input.X * input.X +
				input.Y * input.Y);
			if (inputMagnitude < threshold) {
				float inputScale = inputMagnitude / threshold;
				return input * inputScale;
			}

			return input;
		}
	};

	struct FControllerState
	{
		/** Last frame's button states, so we only send events on edges */
		bool ButtonStates[MAX_NUM_CONTROLLER_BUTTONS];

		/** Next time a repeat event should be generated for each button */
		double NextRepeatTime[MAX_NUM_CONTROLLER_BUTTONS];

		/** Raw Left thumb x analog value */
		int16 LeftXAnalog;

		/** Raw left thumb y analog value */
		int16 LeftYAnalog;

		/** Raw Right thumb x analog value */
		int16 RightXAnalog;

		/** Raw Right thumb x analog value */
		int16 RightYAnalog;

		/** Left Trigger analog value */
		uint8 LeftTriggerAnalog;

		/** Right trigger analog value */
		uint8 RightTriggerAnalog;

		/** If the controller is currently connected */
		bool bIsConnected;

		/** Id of the controller */
		int32 ControllerId;

		/** Current force feedback values */
		FForceFeedbackValues ForceFeedback;

		/** Current force feedback values */
		int64 LeftRumble;

		/** Current force feedback values */
		int64 RightRumble;

		/** Current color intensity of the touchbar */
		float ColorIntensity;

		/** Resistance Type for Triggers. Maps to DS5W:TriggerEffectType */
		uint8 TriggerEffectType;

		/** Bluetooth or USB? */
		bool bIsBluetooth;

		/** The last recorded location of Finger 1 on the touchpad */
		FVector2D LastFirstFingerLocation_Touchpad;

		/** The last recorded location of Finger 2 on the touchpad */
		FVector2D LastSecondFingerLocation_Touchpad;

		FVector4 LightBarState; // R, G, B  (0-255) with W used for intensity (0-1)

		/** What part of the Touchbar should be what color. Note: Unreal maps 0-255 to 0-1, so we're using a Vector here (RGBA). The uint8 maps to DS5W_OSTATE_PLAYER_LED */
		FPlayerLED LEDState;

		float LastLargeValue;
		float LastSmallValue;

		/** IMU sensors processing */
		// for calibration:
		bool UseContinuousCalibration;
		bool CueMotionReset;

		/* Accelerometer */
		FVector Accelerometer;

		/* Gyroscope */
		FVector Gyroscope;

		/* Gamepad orientation */ 
		FQuat Orientation;

		/* Gamepad acceleration */
		FVector Acceleration;

		/* Gravity vector */
		FVector Gravity;

		/* Delta time */
		double LastMeasurementTime;
		double DeltaTime;

		/* Gyroscope axises */
		FGyroscopeSensor GyroscopeAxises;
		FVector2D GyroAxisLastDelta;
	};

	/** If we've been notified by the system that the controller state may have changed */
	bool bNeedsControllerStateUpdate;
	bool bIsGamepadAttached;

	/** In the engine, all controllers map to xbox controllers for consistency */
	uint8 DS5WToXboxControllerMapping[MAX_NUM_CONTROLLER_BUTTONS];

	/** Controller states */
	FControllerState ControllerStates[MAX_NUM_DS5W_CONTROLLERS];

	/** Motion states */
	GamepadMotion MotionStates[MAX_NUM_DS5W_CONTROLLERS];

	/** Delay before sending a repeat message after a button was first pressed */
	float InitialButtonRepeatDelay;

	/** Delay before sending a repeat message after a button has been pressed for a while */
	float ButtonRepeatDelay;

	FName DS5WInterfaceName = "DS5WInterface";
	FString DS5WControllerIdentifier = "DS5WController";

	FGamepadKeyNames::Type Buttons[MAX_NUM_CONTROLLER_BUTTONS];

	int LEDIntToBitMask(FPlayerLED LEDRef);
	DS5W::LedBrightness LEDBrightnessToEnum(FPlayerLED LEDRef);

    /* Message handler */
    TSharedRef<FGenericApplicationMessageHandler>  MessageHandler;
    DS5W::DeviceContext con;

	void reset_continuous_calibration(GamepadMotion& Motion) {
		Motion.ResetContinuousCalibration();
	}

	void push_sensor_samples(GamepadMotion& Motion, const FControllerState& Controller) {
		Motion.ProcessMotion(
			Controller.Gyroscope.X, Controller.Gyroscope.Y, Controller.Gyroscope.Z,
			Controller.Accelerometer.X, Controller.Accelerometer.Y, Controller.Accelerometer.Z,
			Controller.DeltaTime
		);
	}

	void get_calibrated_gyro(FControllerState& Controller, GamepadMotion& Motion)
	{
		Motion.GetCalibratedGyro(Controller.Gyroscope.X, Controller.Gyroscope.Y, Controller.Gyroscope.Z);
	}

	void get_motion_state(FControllerState& Controller, GamepadMotion& Motion)
	{
		Motion.GetProcessedAcceleration(Controller.Acceleration.X, Controller.Acceleration.Y, Controller.Acceleration.Z);
		Motion.GetOrientation(Controller.Orientation.W, Controller.Orientation.X, Controller.Orientation.Y, Controller.Orientation.Z);
		Motion.GetGravity(Controller.Gravity.X, Controller.Gravity.Y, Controller.Gravity.Z);
	}
};