#pragma once

#include "BackendState.h"

#if defined(DEATH_TARGET_IOS)
#include "Backends/iOS/MetalRenderState.h"
#if defined(__OBJC__)
#include <Metal/Metal.h>
#endif
#else
#include "../GL/GLClearColor.h"
#include "../GL/GLViewport.h"
#include "../GL/GLScissorTest.h"
#include "../GL/GLBlending.h"
#include "../GL/GLDepthTest.h"
#include "../GL/GLDebug.h"
#include "GL/GLMapping.h"
#endif

/**
 * Backend façade for render-state related helpers.
 */
namespace nCine::Backend
{
	using ::nCine::ClearColorState;
	using ::nCine::ViewportState;
	using ::nCine::ScissorState;

	// Clear color
	inline ClearColorState GetClearColorState() {
#if defined(DEATH_TARGET_IOS)
		return { Backends::MetalRenderState::clearColor() };
#else
		return { GLClearColor::GetColor() };
#endif
	}
	inline void SetClearColor(const Colorf& color) {
#if defined(DEATH_TARGET_IOS)
		Backends::MetalRenderState::setClearColor(color);
#else
		GLClearColor::SetColor(color);
#endif
	}
	inline void SetClearColorState(const ClearColorState& state) {
#if defined(DEATH_TARGET_IOS)
		Backends::MetalRenderState::setClearColor(state.color);
#else
		GLClearColor::SetColor(state.color);
#endif
	}

	// Viewport
	inline ViewportState GetViewportState() {
#if defined(DEATH_TARGET_IOS)
		return Backends::MetalRenderState::viewport();
#else
		GLViewport::State state = GLViewport::GetState();
		return { state.rect.X, state.rect.Y, state.rect.W, state.rect.H };
#endif
	}
	inline void SetViewportState(const ViewportState& state) {
#if defined(DEATH_TARGET_IOS)
		Backends::MetalRenderState::setViewport(state);
#else
		GLViewport::SetRect(state.x, state.y, state.w, state.h);
#endif
	}
	inline void SetViewportRect(int x, int y, int w, int h) {
#if defined(DEATH_TARGET_IOS)
		Backends::MetalRenderState::setViewport({ x, y, w, h });
#else
		GLViewport::SetRect(x, y, w, h);
#endif
	}

	// Scissor
	inline ScissorState GetScissorState() {
#if defined(DEATH_TARGET_IOS)
		return Backends::MetalRenderState::scissor();
#else
		GLScissorTest::State state = GLScissorTest::GetState();
		return { state.enabled, state.rect.X, state.rect.Y, state.rect.W, state.rect.H };
#endif
	}
	inline void SetScissorState(const ScissorState& state) {
#if defined(DEATH_TARGET_IOS)
		Backends::MetalRenderState::setScissor(state);
#else
		if (state.enabled) {
			GLScissorTest::Enable(state.x, state.y, state.w, state.h);
		} else {
			GLScissorTest::Disable();
		}
#endif
	}
	inline void EnableScissor(int x, int y, int w, int h) {
#if defined(DEATH_TARGET_IOS)
		Backends::MetalRenderState::setScissor({ true, x, y, w, h });
#else
		GLScissorTest::Enable(x, y, w, h);
#endif
	}
	inline void DisableScissor() {
#if defined(DEATH_TARGET_IOS)
		ScissorState state = Backends::MetalRenderState::scissor();
		state.enabled = false;
		Backends::MetalRenderState::setScissor(state);
#else
		GLScissorTest::Disable();
#endif
	}

	// Blending
	inline void EnableBlending() {
#if defined(DEATH_TARGET_IOS)
		// Handled via PSO
#else
		GLBlending::Enable();
#endif
	}
	inline void DisableBlending() {
#if defined(DEATH_TARGET_IOS)
		// Handled via PSO
#else
		GLBlending::Disable();
#endif
	}
	inline void SetBlendFunc(BlendingFactor srcFactor, BlendingFactor dstFactor) {
#if defined(DEATH_TARGET_IOS)
		Backends::MetalRenderState::setBlending(true, srcFactor, dstFactor);
#else
		GLBlending::SetBlendFunc(GLMapping::BlendingFactor(srcFactor), GLMapping::BlendingFactor(dstFactor));
#endif
	}

	// Depth test + depth mask
	inline void DisableDepthMask() {
#if defined(DEATH_TARGET_IOS)
		Backends::MetalRenderState::setDepthMask(false);
#else
		GLDepthTest::DisableDepthMask();
#endif
	}
	inline void EnableDepthMask() {
#if defined(DEATH_TARGET_IOS)
		Backends::MetalRenderState::setDepthMask(true);
#else
		GLDepthTest::EnableDepthMask();
#endif
	}

	// Debug
	inline void DebugReset() {
#if !defined(DEATH_TARGET_IOS)
		GLDebug::Reset();
#endif
	}
	inline void PushDebugGroup(const char* label) {
#if defined(DEATH_TARGET_IOS)
#	if defined(__OBJC__)
		id<MTLRenderCommandEncoder> encoder = (__bridge id<MTLRenderCommandEncoder>)Backends::MetalRenderState::currentEncoder();
		if (encoder != nil) {
			[encoder pushDebugGroup:[NSString stringWithUTF8String:label]];
		}
#	else
		(void)label;
#	endif
#else
		GLDebug::PushGroup(label);
#endif
	}
	inline void PopDebugGroup() {
#if defined(DEATH_TARGET_IOS)
#	if defined(__OBJC__)
		id<MTLRenderCommandEncoder> encoder = (__bridge id<MTLRenderCommandEncoder>)Backends::MetalRenderState::currentEncoder();
		if (encoder != nil) {
			[encoder popDebugGroup];
		}
#	endif
#else
		GLDebug::PopGroup();
#endif
	}

	struct ScopedDebugGroup
	{
		explicit ScopedDebugGroup(const char* label) { PushDebugGroup(label); }
		~ScopedDebugGroup() { PopDebugGroup(); }
	};

	// Clear primitive
	inline void Clear(std::uint32_t mask) {
#if defined(DEATH_TARGET_IOS)
		// Metal clears are typically handled at the start of a render pass via loadAction = MTLLoadActionClear
#else
		glClear(mask);
#endif
	}
}


