#pragma once

#include "../../Input/IInputManager.h"
#include <Shared/Containers/String.h>
#include <Shared/Threading/Spinlock.h>

namespace nCine::Backends
{
	/// Simulated information about iOS keyboard state
	class IosKeyboardState : public KeyboardState
	{
	public:
		IosKeyboardState()
		{
			for (unsigned int i = 0; i < NumKeys; i++) {
				keys_[i] = 0;
			}
		}

		inline bool isKeyDown(Keys key) const override
		{
			return (key != Keys::Unknown && keys_[static_cast<unsigned int>(key)] != 0);
		}

	private:
		static const unsigned int NumKeys = static_cast<unsigned int>(Keys::Count);
		unsigned char keys_[NumKeys];

		friend class IosInputManager;
	};

	/// Information about iOS mouse state
	class IosMouseState : public MouseState
	{
	public:
		IosMouseState() : buttonState_(0) {}

		bool isButtonDown(MouseButton button) const override;

	private:
		int buttonState_;

		friend class IosInputManager;
	};

	/// Information about iOS joystick state
	class IosJoystickState : public JoystickState
	{
	public:
		IosJoystickState() : deviceId_(-1) {}

		bool isButtonPressed(int buttonId) const override { return false; }
		unsigned char hatState(int hatId) const override { return 0; }
		float axisValue(int axisId) const override { return 0.0f; }

	private:
		int deviceId_;

		friend class IosInputManager;
	};

	/// The iOS input manager
	class IosInputManager : public IInputManager
	{
	public:
		/// Touch event types
		enum TouchEventType {
			TOUCHES_BEGAN = 0,
			TOUCHES_MOVED = 1,
			TOUCHES_ENDED = 2,
			TOUCHES_CANCELLED = 3
		};

		IosInputManager();
		~IosInputManager() override;

		bool isJoyPresent(int joyId) const override { return false; }
		const char* joyName(int joyId) const override { return nullptr; }
		const JoystickGuid joyGuid(int joyId) const override { return JoystickGuidType::Unknown; }
		int joyNumButtons(int joyId) const override { return -1; }
		int joyNumHats(int joyId) const override { return -1; }
		int joyNumAxes(int joyId) const override { return -1; }
		const JoystickState& joystickState(int joyId) const override { return nullJoystickState_; }
		bool joystickRumble(int joyId, float lowFreqIntensity, float highFreqIntensity, uint32_t durationMs) override { return false; }
		bool joystickRumbleTriggers(int joyId, float left, float right, uint32_t durationMs) override { return false; }

		const KeyboardState& keyboardState() const override { return keyboardState_; }
		const MouseState& mouseState() const override { return mouseState_; }

		void setCursor(Cursor cursor) override {}

		/// Handles a touch event
		static void HandleTouch(int type, int x, int y, int pointerId, float pressure, float majorRadius);

		/// Handles a key event
		static void HandleKey(int keyCode, bool isDown);

	private:
		static const int MaxPointers = 10;
		static IosKeyboardState keyboardState_;
		static IosMouseState mouseState_;
		static IosJoystickState nullJoystickState_;

		static TouchEvent touchEvent_;
		static Death::Threading::Spinlock touchSpinlock_;
	};
}
