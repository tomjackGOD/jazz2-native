#include "IosInputManager.h"
#include "../../Input/IInputEventHandler.h"
#include "../../Application.h"

namespace nCine::Backends
{
	IosKeyboardState IosInputManager::keyboardState_;
	IosMouseState IosInputManager::mouseState_;
	IosJoystickState IosInputManager::nullJoystickState_;
	TouchEvent IosInputManager::touchEvent_;
	Death::Threading::Spinlock IosInputManager::touchSpinlock_;

	IosInputManager::IosInputManager()
	{
	}

	IosInputManager::~IosInputManager()
	{
	}

	bool IosMouseState::isButtonDown(MouseButton button) const
	{
		return (buttonState_ & (1 << static_cast<int>(button))) != 0;
	}

	void IosInputManager::HandleTouch(int type, int x, int y, int pointerId, float pressure, float majorRadius)
	{
		if (inputEventHandler_ == nullptr || type < TOUCHES_BEGAN || type > TOUCHES_CANCELLED) {
			return;
		}

		touchSpinlock_.lock();

		Vector2i resolution = theApplication().GetResolution();
		const float invW = (resolution.X > 0 ? 1.0f / static_cast<float>(resolution.X) : 0.0f);
		const float invH = (resolution.Y > 0 ? 1.0f / static_cast<float>(resolution.Y) : 0.0f);
		const float normalizedX = static_cast<float>(x) * invW;
		const float normalizedY = static_cast<float>(y) * invH;

		// Find if this pointer is already tracked
		int pointerIndex = -1;
		for (unsigned int i = 0; i < touchEvent_.count; i++) {
			if (touchEvent_.pointers[i].id == pointerId) {
				pointerIndex = i;
				break;
			}
		}

		if (type == TOUCHES_BEGAN) {
			// Ignore duplicate begin events for an existing pointer
			if (pointerIndex != -1) {
				touchSpinlock_.unlock();
				return;
			}
			if (touchEvent_.count < MaxPointers) {
				pointerIndex = static_cast<int>(touchEvent_.count);
				touchEvent_.count++;
			} else {
				touchSpinlock_.unlock();
				return; // Max pointers reached
			}
		} else if (pointerIndex == -1) {
			touchSpinlock_.unlock();
			return; // Pointer not found for Moved/Ended
		}

		float resolvedPressure = pressure;
		if (resolvedPressure < 0.0f) {
			resolvedPressure = 0.0f;
		}
		// iOS does not always provide force (e.g. non-3D-touch hardware), so
		// derive a conservative normalized proxy from touch major radius.
		if (resolvedPressure <= 0.0f && majorRadius > 0.0f) {
			resolvedPressure = majorRadius / 48.0f;
		}
		if (resolvedPressure > 1.0f) {
			resolvedPressure = 1.0f;
		}
		if (type == TOUCHES_ENDED || type == TOUCHES_CANCELLED) {
			resolvedPressure = 0.0f;
		}

		TouchEvent::Pointer& pointer = touchEvent_.pointers[pointerIndex];
		pointer.id = pointerId;
		pointer.x = normalizedX;
		pointer.y = normalizedY;
		pointer.pressure = resolvedPressure;

		touchEvent_.actionIndex = pointerIndex;

		switch (type) {
			case TOUCHES_BEGAN:
				touchEvent_.type = (touchEvent_.count == 1 ? nCine::TouchEventType::Down : nCine::TouchEventType::PointerDown);
				inputEventHandler_->OnTouchEvent(touchEvent_);
				break;
			case TOUCHES_MOVED:
				touchEvent_.type = nCine::TouchEventType::Move;
				inputEventHandler_->OnTouchEvent(touchEvent_);
				break;
			case TOUCHES_ENDED:
			case TOUCHES_CANCELLED:
				touchEvent_.type = (touchEvent_.count == 1 ? nCine::TouchEventType::Up : nCine::TouchEventType::PointerUp);
				inputEventHandler_->OnTouchEvent(touchEvent_);

				// Remove the released pointer after dispatch, mirroring Android semantics
				for (unsigned int i = pointerIndex; i < touchEvent_.count - 1; i++) {
					touchEvent_.pointers[i] = touchEvent_.pointers[i + 1];
				}
				touchEvent_.count--;
				touchEvent_.actionIndex = -1;
				break;
		}

		// Map first touch to left mouse button
		if (touchEvent_.count > 0) {
			mouseState_.x = static_cast<int>(touchEvent_.pointers[0].x * static_cast<float>(resolution.X));
			mouseState_.y = static_cast<int>(touchEvent_.pointers[0].y * static_cast<float>(resolution.Y));
			mouseState_.buttonState_ = (1 << static_cast<int>(MouseButton::Left));
		} else {
			mouseState_.buttonState_ = 0;
		}

		touchSpinlock_.unlock();
	}

	void IosInputManager::HandleKey(int keyCode, bool isDown)
	{
		// keyCode is expected to be a value from the nCine::Keys enum
		if (keyCode < 0 || keyCode >= static_cast<int>(Keys::Count) || keyCode == static_cast<int>(Keys::Unknown)) {
			return;
		}

		keyboardState_.keys_[keyCode] = (isDown ? 1 : 0);

		if (inputEventHandler_ != nullptr) {
			KeyboardEvent event;
			event.sym = static_cast<Keys>(keyCode);
			// On iOS, we map the enum index as a fake scancode for simulated keys
			event.scancode = keyCode;
			event.mod = 0;
			if (isDown) {
				inputEventHandler_->OnKeyPressed(event);
			} else {
				inputEventHandler_->OnKeyReleased(event);
			}
		}
	}
}

extern "C" {
	void ios_bridge_handle_touch(int type, int x, int y, int pointerId, float pressure, float majorRadius) {
		nCine::Backends::IosInputManager::HandleTouch(type, x, y, pointerId, pressure, majorRadius);
	}

	void ios_bridge_handle_key(int keyCode, bool isDown) {
		nCine::Backends::IosInputManager::HandleKey(keyCode, isDown);
	}
}
