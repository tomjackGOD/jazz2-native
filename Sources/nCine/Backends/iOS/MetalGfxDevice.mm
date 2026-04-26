#include "MetalGfxDevice.h"
#include "IosBridge.h"
#include "MetalRenderState.h"
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

namespace nCine::Backends
{
	static id<MTLDevice> _device = nil;
	static id<MTLCommandQueue> _commandQueue = nil;
	static id<MTLCommandBuffer> _currentCommandBuffer = nil;
	static id<CAMetalDrawable> _currentDrawable = nil;
	static id<MTLRenderCommandEncoder> _currentEncoder = nil;

	char MetalGfxDevice::monitorName_[MaxMonitorNameLength];

	MetalGfxDevice::MetalGfxDevice(const WindowMode& windowMode, const DisplayMode& displayMode)
		: IGfxDevice(windowMode, GLContextInfo(), displayMode)
	{
		updateMonitors();
		initDevice();
	}

	MetalGfxDevice::~MetalGfxDevice()
	{
		_commandQueue = nil;
		_device = nil;
	}

	void MetalGfxDevice::update()
	{
		Backends::MetalRenderState::resetTransientBuffers();

		// Finalize the previous frame (if any)
		if (_currentEncoder != nil) {
			[_currentEncoder endEncoding];
			_currentEncoder = nil;
			MetalRenderState::setCurrentEncoder(nullptr);
		}
		if (_currentCommandBuffer != nil && _currentDrawable != nil) {
			[_currentCommandBuffer presentDrawable:_currentDrawable];
			[_currentCommandBuffer commit];
			_currentCommandBuffer = nil;
			_currentDrawable = nil;
		}

		// Begin encoding for the next frame so engine draws can use it between update() calls.
		CAMetalLayer* metalLayer = (__bridge CAMetalLayer*)IosBridge::GetMetalLayer();
		if (metalLayer == nil) {
			return;
		}

		_currentDrawable = [metalLayer nextDrawable];
		if (_currentDrawable == nil) {
			return;
		}

		// Ensure we have a command queue before creating a command buffer
		if (_commandQueue == nil) {
			initDevice();
		}
		_currentCommandBuffer = (_commandQueue != nil) ? [_commandQueue commandBuffer] : nil;
		if (_currentCommandBuffer == nil) {
			_currentDrawable = nil;
			return;
		}

		Colorf clear = MetalRenderState::clearColor();
		MTLRenderPassDescriptor* renderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
		MTLRenderPassColorAttachmentDescriptor* colorAttachment = renderPassDescriptor.colorAttachments[0];
		colorAttachment.texture = _currentDrawable.texture;
		colorAttachment.loadAction = MTLLoadActionClear;
		colorAttachment.storeAction = MTLStoreActionStore;
		colorAttachment.clearColor = MTLClearColorMake(clear.R, clear.G, clear.B, clear.A);

		_currentEncoder = [_currentCommandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
		MetalRenderState::setCurrentEncoder((__bridge void*)_currentEncoder);
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
