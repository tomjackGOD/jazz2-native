#pragma once

#include <cstdint>
#include <string>
#include <Shared/Containers/StringView.h>
#include <Shared/Containers/SmallVector.h>
#include <Shared/Containers/String.h>
#include <Shared/Containers/StringConcatenable.h>
#include "BackendEnums.h"
#include "../../Base/StaticHashMap.h"

namespace nCine
{
	class MetalBufferObject;

	/// Caches a uniform value for Metal compatibility
	class MetalUniformCache
	{
	public:
		MetalUniformCache() : dataPointer_(nullptr), isDirty_(false) {}
		void SetDataPointer(std::uint8_t* dataPointer) { dataPointer_ = dataPointer; }
		bool IsDirty() const { return isDirty_; }
		void SetDirty(bool isDirty) { isDirty_ = isDirty; }
		
		bool SetFloatVector(const float* vec) {
			if (dataPointer_ != nullptr) {
				memcpy(dataPointer_, vec, 16 * sizeof(float)); // assuming 4x4 matrix
				isDirty_ = true;
				return true;
			}
			return false;
		}
		bool SetFloatValue(float v0) { if (dataPointer_) { *(float*)dataPointer_ = v0; isDirty_ = true; return true; } return false; }
		bool SetFloatValue(float v0, float v1) { if (dataPointer_) { ((float*)dataPointer_)[0] = v0; ((float*)dataPointer_)[1] = v1; isDirty_ = true; return true; } return false; }
		bool SetFloatValue(float v0, float v1, float v2) { if (dataPointer_) { ((float*)dataPointer_)[0] = v0; ((float*)dataPointer_)[1] = v1; ((float*)dataPointer_)[2] = v2; isDirty_ = true; return true; } return false; }
		bool SetFloatValue(float v0, float v1, float v2, float v3) { if (dataPointer_) { ((float*)dataPointer_)[0] = v0; ((float*)dataPointer_)[1] = v1; ((float*)dataPointer_)[2] = v2; ((float*)dataPointer_)[3] = v3; isDirty_ = true; return true; } return false; }
		
		bool SetIntVector(const int* vec) { isDirty_ = true; return true; }
		bool SetIntValue(int v0) { isDirty_ = true; return true; }
		bool SetIntValue(int v0, int v1) { isDirty_ = true; return true; }
		bool SetIntValue(int v0, int v1, int v2) { isDirty_ = true; return true; }
		bool SetIntValue(int v0, int v1, int v2, int v3) { isDirty_ = true; return true; }
		bool CommitValue() { isDirty_ = false; return true; }

	private:
		std::uint8_t* dataPointer_;
		bool isDirty_;
	};

	/// Handles a uniform block for Metal
	class MetalUniformBlockCache
	{
	public:
		MetalUniformBlockCache() : dataPointer_(nullptr), size_(0), usedSize_(0), isDirty_(false) {
			mockBlock_.cache = this;
		}
		MetalUniformBlockCache(const MetalUniformBlockCache& other) : dataPointer_(other.dataPointer_), size_(other.size_), usedSize_(other.usedSize_), isDirty_(other.isDirty_), name_(other.name_), uniforms_(other.uniforms_) {
			mockBlock_.cache = this;
		}
		MetalUniformBlockCache& operator=(const MetalUniformBlockCache& other) {
			dataPointer_ = other.dataPointer_;
			size_ = other.size_;
			usedSize_ = other.usedSize_;
			isDirty_ = other.isDirty_;
			name_ = other.name_;
			uniforms_ = other.uniforms_;
			mockBlock_.cache = this;
			return *this;
		}

		void SetDataPointer(std::uint8_t* dataPointer, std::uint32_t size) { dataPointer_ = dataPointer; size_ = size; usedSize_ = size; }
		void SetName(const char* name) { name_ = name; }
		
		MetalUniformCache* GetUniform(const char* name) {
			auto it = uniforms_.find(Death::Containers::String::nullTerminatedView(name));
			return (it != uniforms_.end()) ? &it->second : nullptr;
		}
		
		void AddUniform(const char* name, std::uint32_t offset) {
			MetalUniformCache cache;
			if (dataPointer_) cache.SetDataPointer(dataPointer_ + offset);
			uniforms_[name] = cache;
		}

		bool IsDirty() const { return isDirty_; }
		void SetDirty(bool isDirty) { isDirty_ = isDirty; }
		std::uint8_t* GetDataPointer() const { return dataPointer_; }
		std::uint32_t GetSize() const { return size_; }
		std::uint32_t GetAlignAmount() const { return 0; }
		std::uint32_t usedSize() const { return usedSize_; }
		void SetUsedSize(std::uint32_t usedSize) { usedSize_ = usedSize <= size_ ? usedSize : size_; }

		bool CopyData(std::uint32_t offset, const std::uint8_t* src, std::uint32_t length) {
			if (dataPointer_ != nullptr && offset + length <= size_) {
				memcpy(dataPointer_ + offset, src, length);
				isDirty_ = true;
				return true;
			}
			return false;
		}
		bool CopyData(const std::uint8_t* src) { return CopyData(0, src, usedSize_); }

		struct MockUniformBlock {
			MetalUniformBlockCache* cache;
			const char* GetName() const { return cache->name_.data(); }
		};
		const MockUniformBlock* uniformBlock() const { return &mockBlock_; }

	private:
		std::uint8_t* dataPointer_;
		std::uint32_t size_;
		std::uint32_t usedSize_;
		bool isDirty_;
		Death::Containers::String name_;
		StaticHashMap<Death::Containers::String, MetalUniformCache, 16> uniforms_;
		MockUniformBlock mockBlock_;
	};

	/// Handles all the uniforms of a Metal shader program
	class MetalShaderUniforms
	{
	public:
		using UniformHashMapType = StaticHashMap<Death::Containers::String, MetalUniformCache, 16>;

		MetalShaderUniforms() : shaderProgram_(nullptr) {}
		void SetProgram(class MetalShaderProgram* shaderProgram, const char* includeOnly, const char* exclude);
		void SetUniformsDataPointer(std::uint8_t* dataPointer);
		void SetDirty(bool isDirty);
		bool HasUniform(const char* name) const {
			return (uniformCaches_.find(Death::Containers::String::nullTerminatedView(name)) != uniformCaches_.end());
		}
		MetalUniformCache* GetUniform(const char* name) {
			auto it = uniformCaches_.find(Death::Containers::String::nullTerminatedView(name));
			return (it != uniformCaches_.end()) ? &it->second : nullptr;
		}
		const UniformHashMapType& GetAllUniforms() const { return uniformCaches_; }
		void CommitUniforms();
		std::uint32_t GetUniformCount() const { return static_cast<std::uint32_t>(uniformCaches_.size()); }

	private:
		class MetalShaderProgram* shaderProgram_;
		UniformHashMapType uniformCaches_;
	};

	/// Handles all the uniform blocks of a Metal shader program
	class MetalShaderUniformBlocks
	{
	public:
		using UniformHashMapType = StaticHashMap<Death::Containers::String, MetalUniformBlockCache, 4>;
		MetalShaderUniformBlocks() : shaderProgram_(nullptr) {}
		void SetProgram(class MetalShaderProgram* shaderProgram);
		void SetUniformsDataPointer(std::uint8_t* dataPointer);
		void SetDirty(bool isDirty) {}
		bool HasUniformBlock(const char* name) const {
			return (uniformBlocks_.find(Death::Containers::String::nullTerminatedView(name)) != uniformBlocks_.end());
		}
		MetalUniformBlockCache* GetUniformBlock(const char* name) {
			auto it = uniformBlocks_.find(Death::Containers::String::nullTerminatedView(name));
			return (it != uniformBlocks_.end()) ? &it->second : nullptr;
		}
		const UniformHashMapType& GetAllUniformBlocks() const { return uniformBlocks_; }
		void CommitUniformBlocks() {}
		void Bind();

	private:
		class MetalShaderProgram* shaderProgram_;
		UniformHashMapType uniformBlocks_;
	};

	/// Handles Metal shader programs and pipeline states
	class MetalShaderProgram
	{
	public:
		enum class Introspection { Enabled, NoUniformsInBlocks, Disabled };
		enum class Status { NotLinked, CompilationFailed, LinkingFailed, Linked, LinkedWithIntrospection };
		enum class QueryPhase { Immediate, Deferred };

		static constexpr std::int32_t DefaultBatchSize = -1;

		explicit MetalShaderProgram(QueryPhase queryPhase = QueryPhase::Immediate);
		~MetalShaderProgram();

		void* GetMetalHandle() const { return metalHandle_; }
		std::uint64_t GetGLHandle() const { return reinterpret_cast<std::uintptr_t>(metalHandle_); }
		Status GetStatus() const { return status_; }
		std::uint32_t GetBatchSize() const { return batchSize_; }
		void SetBatchSize(std::uint32_t value) { batchSize_ = value; }
		std::int32_t GetAttributeCount() const { return static_cast<std::int32_t>(attributes_.size()); }

		bool IsLinked() const { return status_ == Status::Linked || status_ == Status::LinkedWithIntrospection; }

		bool AttachShaderFromString(int type, Death::Containers::StringView source);
		bool AttachShaderFromFile(int type, Death::Containers::StringView filename);
		bool AttachShaderFromStringsAndFile(int type, Death::Containers::ArrayView<const Death::Containers::StringView> strings, Death::Containers::StringView filename);
		bool Link(Introspection introspection);
		void Use();
		void Reset();

		std::uint32_t GetUniformsSize() const { return uniformsSize_; }
		void SetUniformsSize(std::uint32_t size) { uniformsSize_ = size; }
		std::uint32_t GetUniformBlocksSize() const { return uniformBlocksSize_; }
		void SetUniformBlocksSize(std::uint32_t size) { uniformBlocksSize_ = size; }

		bool DefineAttribute(const char* name, std::int32_t stride, void* pointer);
		void DefineDefaultAttributes(const char* posName, const char* texName, const char* indexName);
		void DefineVertexFormat(const MetalBufferObject* vbo, const MetalBufferObject* ibo, std::uint32_t vboOffset);

		void SetObjectLabel(Death::Containers::StringView label);

		void* GetVertexFunction() const { return vertexFunction_; }
		void* GetFragmentFunction() const { return fragmentFunction_; }
		void* GetVertexDescriptor() const { return vertexDescriptor_; }
		void* GetPipelineState(bool blendingEnabled, BlendingFactor srcFactor, BlendingFactor destFactor);

		std::uint32_t RetrieveInfoLogLength() const { return 0; }
		void RetrieveInfoLog(std::string& infoLog) const {}
		bool GetLogOnErrors() const { return true; }
		void SetLogOnErrors(bool shouldLogOnErrors) {}

	private:
		void* metalHandle_;
		void* vertexFunction_;
		void* fragmentFunction_;
		void* vertexDescriptor_;

		Death::Containers::String label_;
		
		struct PipelineStateKey {
			bool blendingEnabled;
			BlendingFactor srcFactor;
			BlendingFactor destFactor;

			bool operator==(const PipelineStateKey& other) const {
				return blendingEnabled == other.blendingEnabled && srcFactor == other.srcFactor && destFactor == other.destFactor;
			}
		};
		struct PipelineStateKeyHash {
			std::size_t operator()(const PipelineStateKey& key) const {
				return (static_cast<std::size_t>(key.blendingEnabled) << 16) | (static_cast<std::size_t>(key.srcFactor) << 8) | static_cast<std::size_t>(key.destFactor);
			}
		};
		StaticHashMap<PipelineStateKey, void*, 8> pipelineStates_;

		Status status_;
		std::uint32_t batchSize_;
		std::uint32_t uniformsSize_;
		std::uint32_t uniformBlocksSize_;

		struct AttributeInfo {
			int slot;
			int offset;
			int stride;
			int components;
		};
		Death::Containers::StaticHashMap<Death::Containers::String, AttributeInfo, 16> attributes_;

		MetalShaderProgram(const MetalShaderProgram&) = delete;
		MetalShaderProgram& operator=(const MetalShaderProgram&) = delete;
	};

	using BackendShaderProgram = MetalShaderProgram;
	using BackendShaderUniforms = MetalShaderUniforms;
	using BackendShaderUniformBlocks = MetalShaderUniformBlocks;
}
