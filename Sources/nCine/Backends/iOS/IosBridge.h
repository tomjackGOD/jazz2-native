#pragma once

#include <Shared/Containers/String.h>
#include <Shared/Containers/StringView.h>

using namespace Death;

namespace nCine::Backends
{
	/** @brief A bridge class that provides access to iOS-specific functionality */
	class IosBridge
	{
	public:
		/** @brief Initializes the bridge */
		static void Init();

		/** @brief Returns the preferred language and region code (e.g., "en-US") */
		static String GetPreferredLanguage();

		/** @brief Returns true if the screen is round */
		static bool IsScreenRound();

		/** @brief Returns true if the application has access to its sandbox (always true on iOS) */
		static bool HasExternalStoragePermission();

		/** @brief Requests access to external storage (no-op on iOS) */
		static void RequestExternalStoragePermission();

		/** @brief Opens the specified URL in the system browser */
		static bool OpenUrl(StringView url);

		/** @brief Returns the screen scale */
		static float GetScreenScale();

		/** @brief Returns the screen width in pixels */
		static int32_t GetScreenWidth();

		/** @brief Returns the screen height in pixels */
		static int32_t GetScreenHeight();

		/** @brief Suspends the application */
		static void Suspend();

		/** @brief Resumes the application */
		static void Resume();

		/** @brief Sets the Metal layer for the graphics device */
		static void SetMetalLayer(void* layer);
		/** @brief Returns the Metal layer for the graphics device */
		static void* GetMetalLayer();

	private:
		static void* metalLayer_;
	};
}
