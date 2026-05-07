#include "MetalBufferObject.h"
#include "MetalGfxDevice.h"
#include "../../tracy.h"

#import <Metal/Metal.h>

namespace nCine
{
	MetalBufferObject::MetalBufferObject(Target target)
		: metalHandle_(nullptr), target_(target), size_(0), mapped_(false)
	{
	}

	MetalBufferObject::~MetalBufferObject()
	{
		if (metalHandle_ != nullptr) {
			id<MTLBuffer> buffer = (__bridge_transfer id<MTLBuffer>)metalHandle_;
			buffer = nil;
		}
	}

	bool MetalBufferObject::Bind() const
	{
		// Metal doesn't have a global bind state like OpenGL.
		// Buffers are bound to specific indices in a render command encoder.
		return true;
	}

	bool MetalBufferObject::Unbind() const
	{
		return true;
	}

	void MetalBufferObject::BufferData(std::size_t size, const void* data, int usage)
	{
		ZoneScopedC(0x81A861);
		
		id<MTLDevice> device = (__bridge id<MTLDevice>)Backends::MetalGfxDevice::device();
		if (device == nil) {
			return;
		}

		if (metalHandle_ != nullptr) {
			id<MTLBuffer> oldBuffer = (__bridge_transfer id<MTLBuffer>)metalHandle_;
			oldBuffer = nil;
			metalHandle_ = nullptr;
		}

		size_ = size;
		if (size_ == 0) {
			return;
		}

		// Use Shared storage mode for CPU-to-GPU data transfer on iOS (Unified Memory Architecture)
		id<MTLBuffer> buffer;
		if (data != nullptr) {
			buffer = [device newBufferWithBytes:data length:size_ options:MTLResourceStorageModeShared];
		} else {
			buffer = [device newBufferWithLength:size_ options:MTLResourceStorageModeShared];
		}
		
		metalHandle_ = (__bridge_retained void*)buffer;
	}

	void MetalBufferObject::BufferSubData(std::size_t offset, std::size_t size, const void* data)
	{
		ZoneScopedC(0x81A861);
		
		if (metalHandle_ == nullptr || data == nullptr || offset + size > size_) {
			return;
		}

		id<MTLBuffer> buffer = (__bridge id<MTLBuffer>)metalHandle_;
		uint8_t* dest = static_cast<uint8_t*>([buffer contents]);
		memcpy(dest + offset, data, size);
		
		// On iOS/iPadOS, MTLResourceStorageModeShared means the buffer is always coherent.
		// No need to call didModifyRange: unless we were using MTLResourceStorageModeManaged (macOS only).
	}

	void MetalBufferObject::BindBufferBase(std::uint32_t index)
	{
		// Metal has no global GL-style indexed buffer binding.
		// Uniform buffers are bound directly on the render encoder.
		(void)index;
	}

	void MetalBufferObject::BindBufferRange(std::uint32_t index, std::size_t offset, std::size_t ptrsize)
	{
		// Keep API parity with GLBufferObject so shared code compiles on iOS.
		// Actual per-draw uniform binding is handled by the Metal backend command path.
		(void)index;
		(void)offset;
		(void)ptrsize;
	}

	void* MetalBufferObject::MapBufferRange(std::size_t offset, std::size_t length, int access)
	{
		if (metalHandle_ == nullptr || offset + length > size_) {
			return nullptr;
		}

		mapped_ = true;
		id<MTLBuffer> buffer = (__bridge id<MTLBuffer>)metalHandle_;
		return static_cast<uint8_t*>([buffer contents]) + offset;
	}

	void MetalBufferObject::FlushMappedBufferRange(std::size_t offset, std::size_t length)
	{
		// No-op on iOS (Shared storage mode is coherent)
	}

	bool MetalBufferObject::Unmap()
	{
		mapped_ = false;
		return true;
	}

	void MetalBufferObject::SetObjectLabel(Death::Containers::StringView label)
	{
		if (metalHandle_ != nullptr) {
			id<MTLBuffer> buffer = (__bridge id<MTLBuffer>)metalHandle_;
			buffer.label = [NSString stringWithUTF8String:label.data()];
		}
	}
}
