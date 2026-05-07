#pragma once

#include <cstdint>
#include <Containers/StringView.h>

namespace nCine
{
	/// Handles Metal buffer objects
	class MetalBufferObject
	{
	public:
		enum class Target {
			Array,
			ElementArray,
			Uniform
		};

		explicit MetalBufferObject(Target target);
		~MetalBufferObject();

		void* GetMetalHandle() const { return metalHandle_; }
		Target GetTarget() const { return target_; }
		std::size_t GetSize() const { return size_; }

		bool Bind() const;
		bool Unbind() const;

		void BufferData(std::size_t size, const void* data, int usage);
		void BufferSubData(std::size_t offset, std::size_t size, const void* data);
		void BindBufferBase(std::uint32_t index);
		void BindBufferRange(std::uint32_t index, std::size_t offset, std::size_t ptrsize);

		void* MapBufferRange(std::size_t offset, std::size_t length, int access);
		void FlushMappedBufferRange(std::size_t offset, std::size_t length);
		bool Unmap();

		void SetObjectLabel(Death::Containers::StringView label);

	private:
		void* metalHandle_;
		Target target_;
		std::size_t size_;
		bool mapped_;

		MetalBufferObject(const MetalBufferObject&) = delete;
		MetalBufferObject& operator=(const MetalBufferObject&) = delete;
	};

	using BackendBufferObject = MetalBufferObject;
}
