#pragma once

#include "../../Graphics/IGfxCapabilities.h"

namespace nCine::Backends
{
	/// Metal-based capabilities adapter to satisfy the existing
	/// `IGfxCapabilities` interface used by buffer allocation logic.
	///
	/// Note: this is currently a conservative fixed-value implementation
	/// intended to unblock the Metal renderer port.
	class MetalGfxCapabilities final : public IGfxCapabilities
	{
	public:
		MetalGfxCapabilities();
		~MetalGfxCapabilities() override = default;

		std::int32_t GetGLVersion(GLVersion version) const override;
		const GLInfoStrings& GetGLInfoStrings() const override;
		std::int32_t GetValue(GLIntValues valueName) const override;
		std::int32_t GetArrayValue(GLArrayIntValues arrayValueName, std::uint32_t index) const override;
		bool HasExtension(GLExtensions extensionName) const override;

	private:
		// Keep returned strings alive for the lifetime of the process.
		static const char VendorString_[16];
		static const char RendererString_[16];
		static const char GlVersionString_[16];
		static const char GlslVersionString_[16];

		GLInfoStrings glInfoStrings_;
		std::int32_t intValues_[static_cast<std::int32_t>(GLIntValues::Count)];
	};
}

