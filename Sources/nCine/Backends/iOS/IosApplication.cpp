#include "IosApplication.h"
#include "IosBridge.h"
#include "IosInputManager.h"
#include "MetalGfxDevice.h"

namespace nCine
{
	IosApplication::IosApplication()
		: Application(), isInitialized_(false)
	{
	}

	IosApplication::~IosApplication()
	{
		Shutdown();
	}

	void IosApplication::Run(CreateAppEventHandlerDelegate createAppEventHandler)
	{
		IosApplication& app = theIosApplication();
		app.createAppEventHandler_ = createAppEventHandler;
		
		// Initialization is now deferred until the first ProcessStep call or managed by AppDelegate
		app.PreInit();
	}

	bool IosApplication::OpenUrl(StringView url)
	{
		return Backends::IosBridge::OpenUrl(url);
	}

	void IosApplication::ProcessStep(float deltaTime)
	{
		if (!isInitialized_) {
			if (createAppEventHandler_ == nullptr) {
				return;
			}
			FATAL_ASSERT_MSG(Init(), "iOS application initialization failed");
		}

		if (isSuspended_) {
			return;
		}

		if (deltaTime <= 0.0f) {
			deltaTime = DEFAULT_FRAME_RATE;
		} else if (deltaTime > MAX_DELTA_TIME) {
			deltaTime = MAX_DELTA_TIME;
		}

		Step(deltaTime);
	}

	void IosApplication::HandleContentBoundsChanged(Recti bounds)
	{
		if (isInitialized_ && bounds.W > 0 && bounds.H > 0) {
			// Update the graphics device and application viewports
			Backends::MetalGfxDevice& gfxDevice = static_cast<Backends::MetalGfxDevice&>(*gfxDevice_);
			gfxDevice.setResolution(false, bounds.W, bounds.H);
			
			screenViewport_->setRect(bounds);
		}
	}

	void IosApplication::Suspend()
	{
		if (isInitialized_ && !isSuspended_) {
			Application::Suspend();
		}
	}

	void IosApplication::Resume()
	{
		if (isInitialized_ && isSuspended_) {
			Application::Resume();
		}
	}

	bool IosApplication::CanShowScreenKeyboard() { return true; }
	bool IosApplication::ToggleScreenKeyboard() { return false; }
	bool IosApplication::ShowScreenKeyboard() { return false; }
	bool IosApplication::HideScreenKeyboard() { return false; }

	void IosApplication::PreInit()
	{
		// Set up the bridge
		Backends::IosBridge::Init();
	}

	bool IosApplication::Init()
	{
		if (isInitialized_) {
			return true;
		}

		// Initialize input manager
		inputManager_ = std::make_unique<Backends::IosInputManager>();

		// Initialize graphics device
		gfxDevice_ = std::make_unique<Backends::MetalGfxDevice>(WindowMode(), DisplayMode());

		// Initialize application
		appEventHandler_ = createAppEventHandler_();
		if (appEventHandler_ == nullptr) {
			return false;
		}
		appEventHandler_->OnPreInitialize(appCfg_);

		// Common initialization
		InitCommon();

		isInitialized_ = true;
		return true;
	}

	void IosApplication::Shutdown()
	{
		if (!isInitialized_) {
			return;
		}

		// Shutdown engine components
		appEventHandler_.reset();
		gfxDevice_.reset();
		inputManager_.reset();

		isInitialized_ = false;
	}

	Application& theApplication()
	{
		static IosApplication application;
		return application;
	}
}
