#pragma once

#include <cstdint>
#include <Containers/StringView.h>

namespace nCine
{
	/// Handles Metal texture objects
	class MetalTexture
	{
	public:
		static constexpr std::uint32_t MaxTextureUnits = 16;

		explicit MetalTexture(int target);
		~MetalTexture();

		void* GetMetalHandle() const { return metalHandle_; }
		void* GetSamplerHandle() const { return samplerHandle_; }
		std::uint64_t GetGLHandle() const { return reinterpret_cast<std::uintptr_t>(metalHandle_); }

		void Bind() const;
		void Bind(std::uint32_t unit) const;
		static void Unbind(std::uint32_t unit);

		void TexStorage2D(int levels, int internalFormat, int width, int height);
		void TexImage2D(int level, int internalFormat, int width, int height, int format, int type, const void* data);
		void TexSubImage2D(int level, int x, int y, int width, int height, int format, int type, const void* data);
		
		void CompressedTexImage2D(int level, int internalFormat, int width, int height, int imageSize, const void* data);
		void CompressedTexSubImage2D(int level, int x, int y, int width, int height, int format, int imageSize, const void* data);

		void TexParameteri(int pname, int param);
		void SetObjectLabel(Death::Containers::StringView label);

	private:
		void* metalHandle_;
		void* samplerHandle_;
		int target_;
		int width_, height_;
		int internalFormat_;
		int mipMapLevels_;

		// Sampler state parameters
		int minFilter_, magFilter_, wrapS_, wrapT_;

		void UpdateSamplerState();

		MetalTexture(const MetalTexture&) = delete;
		MetalTexture& operator=(const MetalTexture&) = delete;
	};

	using BackendTexture = MetalTexture;
}
