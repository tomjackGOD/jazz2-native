#pragma once

#include "../../Primitives/Colorf.h"
#include "../../Graphics/Backend/BackendEnums.h"
#include "../../Graphics/Backend/BackendState.h"

namespace nCine::Backends
{
	/// Metal render state management
	class MetalRenderState
	{
	public:
		static void* currentEncoder();
		static void setCurrentEncoder(void* encoder);

		static void setClearColor(const Colorf& color);
		static Colorf clearColor();

		static void setViewport(const ViewportState& state);
		static ViewportState viewport();

		static void setScissor(const ScissorState& state);
		static ScissorState scissor();

		static void setBlending(bool enabled, BlendingFactor src, BlendingFactor dst);
		
		static void setDepthTest(bool enabled);
		static bool depthTest();
		static void setDepthMask(bool enabled);
		static bool depthMask();

		static void* getDepthStencilState();

		static void* acquireTransientBuffer(std::uint32_t size, std::uint32_t& offset);
		static void resetTransientBuffers();
	};
}
