#pragma once

#include "../../Graphics/IGfxDevice.h"

namespace nCine::Backends
{
	/// The Metal based graphics device for iOS
	class MetalGfxDevice : public IGfxDevice
	{
	public:
		MetalGfxDevice(const WindowMode& windowMode, const DisplayMode& displayMode);
		~MetalGfxDevice() override;

		void setSwapInterval(int interval) override {}
		void setResolution(bool fullscreen, int width = 0, int height = 0) override;
		void setWindowPosition(int x, int y) override {}
		void setWindowSize(int width, int height) override {}

		void update() override;

		void BeginFrame();

		void setWindowTitle(StringView windowTitle) override {}
		void setWindowIcon(StringView windowIconFilename) override {}

		const VideoMode& currentVideoMode(unsigned int monitorIndex) const override;
		bool setVideoMode(unsigned int modeIndex) override;

		/** @brief Returns the Metal device */
		static void* device();
		/** @brief Returns the Metal command queue */
		static void* commandQueue();
		/** @brief Returns the default pixel format for color attachments */
		static unsigned long pixelFormat();

	protected:
		void setResolutionInternal(int width, int height) override {}

	private:
		void initDevice();
		void updateMonitors() override;
		void clearTransientResources();

		void* _drawable;
		void* _commandBuffer;

		static constexpr float DefaultRefreshRate = 60.0f;
		static char monitorName_[MaxMonitorNameLength];
		static const unsigned int MaxMonitorNameLength = 64;
	};
}
