#include "MetalTexture.h"
#include "MetalGfxDevice.h"
#include "../../tracy.h"

#import <Metal/Metal.h>

namespace nCine
{
	namespace
	{
		MTLPixelFormat GetMetalPixelFormat(int internalFormat)
		{
			// These are standard GL internal formats used in the engine
			switch (internalFormat) {
				case 0x8229: return MTLPixelFormatR8Unorm;     // GL_R8
				case 0x822B: return MTLPixelFormatRG8Unorm;    // GL_RG8
				case 0x8051: return MTLPixelFormatRGBA8Unorm;  // GL_RGB8 (Metal doesn't support 24-bit RGB)
				case 0x8058: return MTLPixelFormatRGBA8Unorm;  // GL_RGBA8
				// Compressed formats (PVRTC, ASTC, etc.) would be handled here
				default:     return MTLPixelFormatRGBA8Unorm;
			}
		}

		MTLSamplerMinMagFilter GetMetalFilter(int param)
		{
			switch (param) {
				case 0x2600: return MTLSamplerMinMagFilterNearest; // GL_NEAREST
				case 0x2601: return MTLSamplerMinMagFilterLinear;  // GL_LINEAR
				default:     return MTLSamplerMinMagFilterLinear;
			}
		}

		MTLSamplerMipFilter GetMetalMipFilter(int param)
		{
			switch (param) {
				case 0x2700: return MTLSamplerMipFilterNearest; // GL_NEAREST_MIPMAP_NEAREST
				case 0x2701: return MTLSamplerMipFilterNearest; // GL_LINEAR_MIPMAP_NEAREST
				case 0x2702: return MTLSamplerMipFilterLinear;  // GL_NEAREST_MIPMAP_LINEAR
				case 0x2703: return MTLSamplerMipFilterLinear;  // GL_LINEAR_MIPMAP_LINEAR
				default:     return MTLSamplerMipFilterNotMipmapped;
			}
		}

		MTLSamplerAddressMode GetMetalAddressMode(int param)
		{
			switch (param) {
				case 0x812F: return MTLSamplerAddressModeClampToEdge;   // GL_CLAMP_TO_EDGE
				case 0x8370: return MTLSamplerAddressModeMirrorRepeat; // GL_MIRRORED_REPEAT
				case 0x2901: return MTLSamplerAddressModeRepeat;       // GL_REPEAT
				default:     return MTLSamplerAddressModeClampToEdge;
			}
		}
	}

	MetalTexture::MetalTexture(int target)
		: metalHandle_(nullptr), samplerHandle_(nullptr), target_(target), width_(0), height_(0), 
		  internalFormat_(0), mipMapLevels_(1),
		  minFilter_(0x2601), magFilter_(0x2601), wrapS_(0x812F), wrapT_(0x812F)
	{
	}

	MetalTexture::~MetalTexture()
	{
		if (metalHandle_ != nullptr) {
			id<MTLTexture> texture = (__bridge_transfer id<MTLTexture>)metalHandle_;
			texture = nil;
		}
		if (samplerHandle_ != nullptr) {
			id<MTLSamplerState> sampler = (__bridge_transfer id<MTLSamplerState>)samplerHandle_;
			sampler = nil;
		}
	}

	void MetalTexture::Bind() const
	{
		Bind(0);
	}

	void MetalTexture::Bind(std::uint32_t unit) const
	{
		// Metal doesn't have a global bind state.
		// Textures are bound to indices in a render command encoder.
	}

	void MetalTexture::Unbind(std::uint32_t unit)
	{
	}

	void MetalTexture::TexStorage2D(int levels, int internalFormat, int width, int height)
	{
		ZoneScopedC(0x81A861);
		
		id<MTLDevice> device = (__bridge id<MTLDevice>)Backends::MetalGfxDevice::device();
		if (device == nil) {
			return;
		}

		if (metalHandle_ != nullptr) {
			id<MTLTexture> oldTexture = (__bridge_transfer id<MTLTexture>)metalHandle_;
			oldTexture = nil;
			metalHandle_ = nullptr;
		}

		width_ = width;
		height_ = height;
		internalFormat_ = internalFormat;
		mipMapLevels_ = levels;

		MTLTextureDescriptor* desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:GetMetalPixelFormat(internalFormat)
																						width:static_cast<NSUInteger>(width)
																					   height:static_cast<NSUInteger>(height)
																					mipmapped:(levels > 1)];
		desc.usage = MTLTextureUsageShaderRead;
		desc.storageMode = MTLStorageModeShared;
		
		id<MTLTexture> texture = [device newTextureWithDescriptor:desc];
		metalHandle_ = (__bridge_retained void*)texture;
		
		UpdateSamplerState();
	}

	void MetalTexture::TexImage2D(int level, int internalFormat, int width, int height, int format, int type, const void* data)
	{
		TexStorage2D(1, internalFormat, width, height);
		if (data != nullptr) {
			TexSubImage2D(level, 0, 0, width, height, format, type, data);
		}
	}

	void MetalTexture::TexSubImage2D(int level, int x, int y, int width, int height, int format, int type, const void* data)
	{
		ZoneScopedC(0x81A861);
		
		if (metalHandle_ == nullptr || data == nullptr) {
			return;
		}

		id<MTLTexture> texture = (__bridge id<MTLTexture>)metalHandle_;
		MTLRegion region = MTLRegionMake2D(x, y, width, height);
		
		// Calculate bytes per row (assuming 4 bytes for RGBA8)
		NSUInteger bytesPerRow = width * 4; 
		if (internalFormat_ == 0x8229) bytesPerRow = width * 1; // R8
		else if (internalFormat_ == 0x822B) bytesPerRow = width * 2; // RG8

		[texture replaceRegion:region mipmapLevel:level withBytes:data bytesPerRow:bytesPerRow];
	}

	void MetalTexture::CompressedTexImage2D(int level, int internalFormat, int width, int height, int imageSize, const void* data)
	{
		// TODO: Implement compressed texture support
	}

	void MetalTexture::CompressedTexSubImage2D(int level, int x, int y, int width, int height, int format, int imageSize, const void* data)
	{
		// TODO: Implement compressed texture support
	}

	void MetalTexture::TexParameteri(int pname, int param)
	{
		bool changed = false;
		switch (pname) {
			case 0x2801: if (minFilter_ != param) { minFilter_ = param; changed = true; } break; // GL_TEXTURE_MIN_FILTER
			case 0x2800: if (magFilter_ != param) { magFilter_ = param; changed = true; } break; // GL_TEXTURE_MAG_FILTER
			case 0x2802: if (wrapS_ != param) { wrapS_ = param; changed = true; } break;         // GL_TEXTURE_WRAP_S
			case 0x2803: if (wrapT_ != param) { wrapT_ = param; changed = true; } break;         // GL_TEXTURE_WRAP_T
		}
		
		if (changed) {
			UpdateSamplerState();
		}
	}

	void MetalTexture::SetObjectLabel(Death::Containers::StringView label)
	{
		if (metalHandle_ != nullptr) {
			id<MTLTexture> texture = (__bridge id<MTLTexture>)metalHandle_;
			texture.label = [NSString stringWithUTF8String:label.data()];
		}
	}

	void MetalTexture::UpdateSamplerState()
	{
		id<MTLDevice> device = (__bridge id<MTLDevice>)Backends::MetalGfxDevice::device();
		if (device == nil) {
			return;
		}

		if (samplerHandle_ != nullptr) {
			id<MTLSamplerState> oldSampler = (__bridge_transfer id<MTLSamplerState>)samplerHandle_;
			oldSampler = nil;
			samplerHandle_ = nullptr;
		}

		MTLSamplerDescriptor* desc = [[MTLSamplerDescriptor alloc] init];
		desc.minFilter = GetMetalFilter(minFilter_);
		desc.magFilter = GetMetalFilter(magFilter_);
		desc.mipFilter = GetMetalMipFilter(minFilter_);
		desc.sAddressMode = GetMetalAddressMode(wrapS_);
		desc.tAddressMode = GetMetalAddressMode(wrapT_);
		
		id<MTLSamplerState> sampler = [device newSamplerStateWithDescriptor:desc];
		samplerHandle_ = (__bridge_retained void*)sampler;
	}
}
