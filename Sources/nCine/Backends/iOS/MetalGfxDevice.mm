#include "MetalGfxDevice.h"
#include "IosBridge.h"
#include "MetalRenderState.h"
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

namespace nCine::Backends
{
	static id<MTLDevice> _device = nil;
	static id<MTLCommandQueue> _commandQueue = nil;

	char MetalGfxDevice::monitorName_[MaxMonitorNameLength];

	MetalGfxDevice::MetalGfxDevice(const WindowMode& windowMode, const DisplayMode& displayMode)
		: IGfxDevice(windowMode, GLContextInfo(), displayMode), _drawable(nullptr), _commandBuffer(nullptr)
	{
		updateMonitors();
		initDevice();
	}

	MetalGfxDevice::~MetalGfxDevice()
	{
		clearTransientResources();
		_commandQueue = nil;
		_device = nil;
	}

	void MetalGfxDevice::clearTransientResources()
	{
		if (_commandBuffer != nullptr) {
			id<MTLCommandBuffer> commandBuffer = (__bridge_transfer id<MTLCommandBuffer>)_commandBuffer;
			_commandBuffer = nullptr;
			commandBuffer = nil;
		}
		if (_drawable != nullptr) {
			id<CAMetalDrawable> drawable = (__bridge_transfer id<CAMetalDrawable>)_drawable;
			_drawable = nullptr;
			drawable = nil;
		}
	}

	void MetalGfxDevice::BeginFrame()
	{
		clearTransientResources();
		Backends::MetalRenderState::resetTransientBuffers();

		CAMetalLayer* metalLayer = (__bridge CAMetalLayer*)IosBridge::GetMetalLayer();
		if (metalLayer == nil) {
			return;
		}

		id<CAMetalDrawable> drawable = [metalLayer nextDrawable];
		if (drawable == nil) {
			return;
		}
		_drawable = (__bridge_retained void*)drawable;

		if (_commandQueue == nil) {
			initDevice();
		}

		id<MTLCommandBuffer> commandBuffer = (_commandQueue != nil) ? [_commandQueue commandBuffer] : nil;
		if (commandBuffer == nil) {
			clearTransientResources();
			return;
		}
		_commandBuffer = (__bridge_retained void*)commandBuffer;

		MTLRenderPassDescriptor* renderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
		MTLRenderPassColorAttachmentDescriptor* colorAttachment = renderPassDescriptor.colorAttachments[0];
		colorAttachment.texture = drawable.texture;
		colorAttachment.loadAction = MTLLoadActionClear;
		colorAttachment.storeAction = MTLStoreActionStore;
		
		Colorf clearColor = Backends::MetalRenderState::clearColor();
		colorAttachment.clearColor = MTLClearColorMake(clearColor.R, clearColor.G, clearColor.B, clearColor.A);
		
		id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
		if (encoder == nil) {
			clearTransientResources();
			return;
		}
		Backends::MetalRenderState::setCurrentEncoder((__bridge void*)encoder);
	}

	void MetalGfxDevice::update()
	{
		id<MTLRenderCommandEncoder> encoder = (__bridge id<MTLRenderCommandEncoder>)Backends::MetalRenderState::currentEncoder();
		if (encoder != nil) {
			[encoder endEncoding];
			Backends::MetalRenderState::setCurrentEncoder(nullptr);
		}

		if (_commandBuffer != nullptr) {
			id<MTLCommandBuffer> commandBuffer = (__bridge_transfer id<MTLCommandBuffer>)_commandBuffer;
			_commandBuffer = nullptr;

			if (_drawable != nullptr) {
				id<CAMetalDrawable> drawable = (__bridge_transfer id<CAMetalDrawable>)_drawable;
				_drawable = nullptr;
				[commandBuffer presentDrawable:drawable];
				drawable = nil;
			}

			[commandBuffer commit];
			commandBuffer = nil;
		}
	}

	const IGfxDevice::VideoMode& MetalGfxDevice::currentVideoMode(unsigned int monitorIndex) const
	{
		return monitors_[0].videoModes[0];
	}

	bool MetalGfxDevice::setVideoMode(unsigned int modeIndex)
	{
		return false;
	}

	void MetalGfxDevice::setResolution(bool fullscreen, int width, int height)
	{
		// On iOS, we don't change the actual screen resolution, 
		// but we update our internal representation of it.
		if (width > 0 && height > 0) {
			monitors_[0].videoModes[0].width = static_cast<unsigned int>(width);
			monitors_[0].videoModes[0].height = static_cast<unsigned int>(height);
			
			// Keep CAMetalLayer drawable size in sync with the pixel dimensions.
			CAMetalLayer* metalLayer = (__bridge CAMetalLayer*)IosBridge::GetMetalLayer();
			if (metalLayer != nil) {
				metalLayer.drawableSize = CGSizeMake(static_cast<CGFloat>(width), static_cast<CGFloat>(height));
			}
		}
	}

	void* MetalGfxDevice::device()
	{
		return (__bridge void*)_device;
	}

	void* MetalGfxDevice::commandQueue()
	{
		return (__bridge void*)_commandQueue;
	}

	unsigned long MetalGfxDevice::pixelFormat()
	{
		return (unsigned long)MTLPixelFormatBGRA8Unorm;
	}

	void MetalGfxDevice::initDevice()
	{
		if (_device == nil) {
			_device = MTLCreateSystemDefaultDevice();
			if (_device != nil) {
				_commandQueue = [_device newCommandQueue];
			}
		}
	}

	void MetalGfxDevice::updateMonitors()
	{
		numMonitors_ = 1;
		monitors_[0].name = "iOS Display";
		monitors_[0].position.X = 0;
		monitors_[0].position.Y = 0;

		float scale = IosBridge::GetScreenScale();
		monitors_[0].scale.X = scale;
		monitors_[0].scale.Y = scale;

		monitors_[0].numVideoModes = 1;
		monitors_[0].videoModes[0].width = static_cast<unsigned int>(IosBridge::GetScreenWidth());
		monitors_[0].videoModes[0].height = static_cast<unsigned int>(IosBridge::GetScreenHeight());
		monitors_[0].videoModes[0].refreshRate = DefaultRefreshRate;
	}
}
