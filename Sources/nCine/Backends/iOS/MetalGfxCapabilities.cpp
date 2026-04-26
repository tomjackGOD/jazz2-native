#include "MetalGfxCapabilities.h"

#if defined(DEATH_TARGET_IOS)
#import <Metal/Metal.h>
#endif

namespace nCine::Backends
{
	const char MetalGfxCapabilities::VendorString_[16] = "Apple";
	const char MetalGfxCapabilities::RendererString_[16] = "Metal";
	const char MetalGfxCapabilities::GlVersionString_[16] = "Metal";
	const char MetalGfxCapabilities::GlslVersionString_[16] = "MSL";

	MetalGfxCapabilities::MetalGfxCapabilities()
	{
		for (std::int32_t i = 0; i < static_cast<std::int32_t>(GLIntValues::Count); i++) {
			intValues_[i] = 0;
		}

		// Conservative defaults that work with the current UBO layout sizes.
		// Metal constant buffers are typically aligned to 256 bytes for best compatibility.
		intValues_[static_cast<std::int32_t>(GLIntValues::MAX_TEXTURE_SIZE)] = 8192;
		intValues_[static_cast<std::int32_t>(GLIntValues::MAX_TEXTURE_IMAGE_UNITS)] = 8;
		intValues_[static_cast<std::int32_t>(GLIntValues::MAX_UNIFORM_BLOCK_SIZE)] = 64 * 1024;
		intValues_[static_cast<std::int32_t>(GLIntValues::MAX_UNIFORM_BLOCK_SIZE_NORMALIZED)] = 64 * 1024;
		intValues_[static_cast<std::int32_t>(GLIntValues::MAX_UNIFORM_BUFFER_BINDINGS)] = 24;
		intValues_[static_cast<std::int32_t>(GLIntValues::MAX_VERTEX_UNIFORM_BLOCKS)] = 12;
		intValues_[static_cast<std::int32_t>(GLIntValues::MAX_FRAGMENT_UNIFORM_BLOCKS)] = 12;
		intValues_[static_cast<std::int32_t>(GLIntValues::UNIFORM_BUFFER_OFFSET_ALIGNMENT)] = 256;
		intValues_[static_cast<std::int32_t>(GLIntValues::MAX_VERTEX_ATTRIB_STRIDE)] = 2048;
		intValues_[static_cast<std::int32_t>(GLIntValues::MAX_COLOR_ATTACHMENTS)] = 1;
		intValues_[static_cast<std::int32_t>(GLIntValues::NUM_PROGRAM_BINARY_FORMATS)] = -1;

#if defined(DEATH_TARGET_IOS)
		// Use real device values when possible.
		id<MTLDevice> device = MTLCreateSystemDefaultDevice();
		if (device != nil) {
			// best-effort: max texture size
			intValues_[static_cast<std::int32_t>(GLIntValues::MAX_TEXTURE_SIZE)] =
				static_cast<std::int32_t>(device.maxTextureDimension2D);
		}
#endif

		glInfoStrings_.vendor = VendorString_;
		glInfoStrings_.renderer = RendererString_;
		glInfoStrings_.glVersion = GlVersionString_;
		glInfoStrings_.glslVersion = GlslVersionString_;
	}

	std::int32_t MetalGfxCapabilities::GetGLVersion(GLVersion version) const
	{
		(void)version;
		return 0;
	}

	const GLInfoStrings& MetalGfxCapabilities::GetGLInfoStrings() const
	{
		return glInfoStrings_;
	}

	std::int32_t MetalGfxCapabilities::GetValue(GLIntValues valueName) const
	{
		const std::int32_t idx = static_cast<std::int32_t>(valueName);
		if (idx >= 0 && idx < static_cast<std::int32_t>(GLIntValues::Count)) {
			return intValues_[idx];
		}
		return 0;
	}

	std::int32_t MetalGfxCapabilities::GetArrayValue(GLArrayIntValues arrayValueName, std::uint32_t index) const
	{
		(void)index;
		(void)arrayValueName;
		return 0;
	}

	bool MetalGfxCapabilities::HasExtension(GLExtensions /*extensionName*/) const
	{
		return false;
	}
}

