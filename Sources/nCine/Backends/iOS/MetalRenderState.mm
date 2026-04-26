#include "MetalRenderState.h"
#include "MetalGfxDevice.h"
#import <Metal/Metal.h>

namespace nCine::Backends
{
	static id<MTLRenderCommandEncoder> _currentEncoder = nil;
	static Colorf _clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	static ViewportState _viewport = { 0, 0, 0, 0 };
	static ScissorState _scissor = { false, 0, 0, 0, 0 };
	static bool _depthTestEnabled = false;
	static bool _depthMaskEnabled = true;

	static id<MTLDepthStencilState> _depthStencilStates[4] = { nil, nil, nil, nil };

	static id<MTLBuffer> _transientBuffers[3] = { nil, nil, nil };
	static std::uint32_t _transientBufferIndex = 0;
	static std::uint32_t _transientBufferOffset = 0;
	static constexpr std::uint32_t TransientBufferSize = 1024 * 1024; // 1 MB

	void* MetalRenderState::currentEncoder()
	{
		return (__bridge void*)_currentEncoder;
	}

	void MetalRenderState::setCurrentEncoder(void* encoder)
	{
		_currentEncoder = (__bridge id<MTLRenderCommandEncoder>)encoder;
	}

	void MetalRenderState::setClearColor(const Colorf& color)
	{
		_clearColor = color;
	}

	Colorf MetalRenderState::clearColor()
	{
		return _clearColor;
	}

	void MetalRenderState::setViewport(const ViewportState& state)
	{
		_viewport = state;
		if (_currentEncoder != nil) {
			MTLViewport mtlViewport = {
				static_cast<double>(state.x),
				static_cast<double>(state.y),
				static_cast<double>(state.w),
				static_cast<double>(state.h),
				0.0, 1.0
			};
			[_currentEncoder setViewport:mtlViewport];
		}
	}

	ViewportState MetalRenderState::viewport()
	{
		return _viewport;
	}

	void MetalRenderState::setScissor(const ScissorState& state)
	{
		_scissor = state;
		if (_currentEncoder != nil && state.enabled) {
			MTLScissorRect mtlScissor = {
				static_cast<NSUInteger>(state.x),
				static_cast<NSUInteger>(state.y),
				static_cast<NSUInteger>(state.w),
				static_cast<NSUInteger>(state.h)
			};
			[_currentEncoder setScissorRect:mtlScissor];
		}
	}

	ScissorState MetalRenderState::scissor()
	{
		return _scissor;
	}

	void MetalRenderState::setBlending(bool enabled, BlendingFactor src, BlendingFactor dst)
	{
		// Metal blending is part of the Pipeline State Object.
		// We would need to invalidate the current PSO if blending changes.
	}

	void MetalRenderState::setDepthTest(bool enabled)
	{
		_depthTestEnabled = enabled;
	}

	bool MetalRenderState::depthTest()
	{
		return _depthTestEnabled;
	}

	void MetalRenderState::setDepthMask(bool enabled)
	{
		_depthMaskEnabled = enabled;
	}

	bool MetalRenderState::depthMask()
	{
		return _depthMaskEnabled;
	}

	void* MetalRenderState::getDepthStencilState()
	{
		int index = (_depthTestEnabled ? 2 : 0) | (_depthMaskEnabled ? 1 : 0);
		if (_depthStencilStates[index] != nil) {
			return (__bridge void*)_depthStencilStates[index];
		}

		id<MTLDevice> device = (__bridge id<MTLDevice>)Backends::MetalGfxDevice::device();
		if (device == nil) {
			return nullptr;
		}

		MTLDepthStencilDescriptor* depthDesc = [[MTLDepthStencilDescriptor alloc] init];
		if (_depthTestEnabled) {
			depthDesc.depthCompareFunction = MTLCompareFunctionLessEqual;
		} else {
			depthDesc.depthCompareFunction = MTLCompareFunctionAlways;
		}
		depthDesc.depthWriteEnabled = _depthMaskEnabled ? YES : NO;

		_depthStencilStates[index] = [device newDepthStencilStateWithDescriptor:depthDesc];
		return (__bridge void*)_depthStencilStates[index];
	}

	void* MetalRenderState::acquireTransientBuffer(std::uint32_t size, std::uint32_t& offset)
	{
		// Alignment for constant buffers in Metal is 256 bytes
		_transientBufferOffset = (_transientBufferOffset + 255) & ~255;

		if (_transientBuffers[_transientBufferIndex] == nil || _transientBufferOffset + size > TransientBufferSize) {
			_transientBufferOffset = 0;
			_transientBufferIndex = (_transientBufferIndex + 1) % 3;

			if (_transientBuffers[_transientBufferIndex] == nil) {
				id<MTLDevice> device = (__bridge id<MTLDevice>)Backends::MetalGfxDevice::device();
				_transientBuffers[_transientBufferIndex] = [device newBufferWithLength:TransientBufferSize options:MTLResourceStorageModeShared];
			}
		}

		offset = _transientBufferOffset;
		_transientBufferOffset += size;
		return (__bridge void*)_transientBuffers[_transientBufferIndex];
	}

	void MetalRenderState::resetTransientBuffers()
	{
		_transientBufferOffset = 0;
		// We rotate through 3 buffers to avoid CPU/GPU contention if one is still in use
	}
}
