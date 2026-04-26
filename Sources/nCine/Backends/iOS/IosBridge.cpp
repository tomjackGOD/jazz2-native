#include "IosBridge.h"
#include "IosApplication.h"
#include <nCine/IAppEventHandler.h>

extern "C" {
    const char* ios_bridge_get_preferred_language();
    bool ios_bridge_is_screen_round();
    bool ios_bridge_has_external_storage_permission();
    void ios_bridge_request_external_storage_permission();
    bool ios_bridge_open_url(const char* url);
    void ios_bridge_handle_touch(int type, int x, int y, int pointerId, float pressure, float majorRadius);
    float ios_bridge_get_screen_scale();
    int32_t ios_bridge_get_screen_width();
    int32_t ios_bridge_get_screen_height();

	void ios_bridge_set_metal_layer(void* layer) {
		nCine::Backends::IosBridge::SetMetalLayer(layer);
	}

	std::unique_ptr<nCine::IAppEventHandler> CreateAppEventHandler();

	void ios_bridge_run() {
		nCine::IosApplication::Run(CreateAppEventHandler);
	}

	void ios_bridge_process_frame(float deltaTime) {
		static_cast<nCine::IosApplication&>(nCine::theApplication()).ProcessStep(deltaTime);
	}

	void ios_bridge_handle_resize(int width, int height) {
		static_cast<nCine::IosApplication&>(nCine::theApplication()).HandleContentBoundsChanged(nCine::Recti(0, 0, width, height));
	}

	void ios_bridge_suspend() {
		static_cast<nCine::IosApplication&>(nCine::theApplication()).Suspend();
	}

	void ios_bridge_resume() {
		static_cast<nCine::IosApplication&>(nCine::theApplication()).Resume();
	}
}

namespace nCine::Backends
{
	void* IosBridge::metalLayer_ = nullptr;

    void IosBridge::Init()
    {
    }

    String IosBridge::GetPreferredLanguage()
    {
        const char* lang = ios_bridge_get_preferred_language();
        return (lang != nullptr ? String(lang) : String());
    }

    bool IosBridge::IsScreenRound()
    {
        return ios_bridge_is_screen_round();
    }

    bool IosBridge::HasExternalStoragePermission()
    {
        return ios_bridge_has_external_storage_permission();
    }

    void IosBridge::RequestExternalStoragePermission()
    {
        ios_bridge_request_external_storage_permission();
    }

    bool IosBridge::OpenUrl(StringView url)
    {
        // `StringView::data()` is not guaranteed to be null-terminated.
        // Convert to a null-terminated `String` before passing to Swift.
        String nullTerminatedUrl = String::nullTerminatedView(url);
        return ios_bridge_open_url(nullTerminatedUrl.data());
    }

    float IosBridge::GetScreenScale()
    {
        return ios_bridge_get_screen_scale();
    }

    int32_t IosBridge::GetScreenWidth()
    {
        return ios_bridge_get_screen_width();
    }

    int32_t IosBridge::GetScreenHeight()
    {
        return ios_bridge_get_screen_height();
    }

    void IosBridge::Suspend()
    {
        static_cast<nCine::IosApplication&>(nCine::theApplication()).Suspend();
    }

    void IosBridge::Resume()
    {
        static_cast<nCine::IosApplication&>(nCine::theApplication()).Resume();
    }

    void IosBridge::SetMetalLayer(void* layer)
    {
        metalLayer_ = layer;
    }

    void* IosBridge::GetMetalLayer()
    {
        return metalLayer_;
    }
}
