#include "Geometry.h"
#include "RenderResources.h"
#include "RenderStatistics.h"
#if !defined(DEATH_TARGET_IOS)
#include "GL/GLMapping.h"
#endif

#include <cstring> // for memcpy()

namespace nCine
{
	Geometry::Geometry()
		: primitiveType_(PrimitiveType::Triangles), firstVertex_(0), numVertices_(0), numElementsPerVertex_(2), firstIndex_(0), numIndices_(0),
			hostVertexPointer_(nullptr), hostIndexPointer_(nullptr), vboUsageFlags_(BufferUsage::StaticDraw), sharedVboParams_(nullptr), iboUsageFlags_(BufferUsage::StaticDraw),
			sharedIboParams_(nullptr), hasDirtyVertices_(true), hasDirtyIndices_(true)
	{
	}

	Geometry::~Geometry()
	{
#if defined(NCINE_PROFILING)
		if (vbo_ != nullptr) {
			RenderStatistics::RemoveCustomVbo(vbo_->GetSize());
		}
		if (ibo_ != nullptr) {
			RenderStatistics::RemoveCustomIbo(ibo_->GetSize());
		}
#endif
	}

	void Geometry::SetDrawParameters(PrimitiveType primitiveType, std::int32_t firstVertex, std::int32_t numVertices)
	{
		primitiveType_ = primitiveType;
		firstVertex_ = firstVertex;
		numVertices_ = numVertices;
	}

	void Geometry::CreateCustomVbo(std::uint32_t numFloats, BufferUsage usage)
	{
#if defined(DEATH_TARGET_IOS)
		vbo_ = std::make_unique<BackendBufferObject>(BackendBufferObject::Target::Array);
		vbo_->BufferData(numFloats * sizeof(float), nullptr, 0);
#else
		vbo_ = std::make_unique<BackendBufferObject>(GL_ARRAY_BUFFER);
		vbo_->BufferData(numFloats * sizeof(float), nullptr, GLMapping::BufferUsage(usage));
#endif

		vboUsageFlags_ = usage;
		vboParams_.object = vbo_.get();
		vboParams_.size = static_cast<std::uint32_t>(vbo_->GetSize());
		vboParams_.offset = 0;
		vboParams_.mapBase = nullptr;

#if defined(NCINE_PROFILING)
		RenderStatistics::AddCustomVbo(vbo_->GetSize());
#endif
	}

	float* Geometry::AcquireVertexPointer(std::uint32_t numFloats, std::uint32_t numFloatsAlignment)
	{
		DEATH_ASSERT(vbo_ == nullptr);
		hasDirtyVertices_ = true;

#if defined(DEATH_TARGET_IOS)
		// Metal backend: allocate a dedicated buffer and map it for CPU writes
		CreateCustomVbo(numFloats, BufferUsage::StreamDraw);
		vboParams_.mapBase = static_cast<std::uint8_t*>(vbo_->MapBufferRange(0, vboParams_.size, 0));
		return reinterpret_cast<float*>(vboParams_.mapBase);
#else
		if (sharedVboParams_ != nullptr) {
			vboParams_ = *sharedVboParams_;
		} else {
			const RenderBuffersManager::BufferTypes bufferType = RenderBuffersManager::BufferTypes::Array;
			if (vboParams_.mapBase == nullptr) {
				vboParams_ = RenderResources::GetBuffersManager().AcquireMemory(bufferType, numFloats * sizeof(float), numFloatsAlignment * sizeof(float));
			}
		}

		return reinterpret_cast<float*>(vboParams_.mapBase + vboParams_.offset);
#endif
	}

	/*! This method can only be used when mapping of OpenGL buffers is available */
	float* Geometry::AcquireVertexPointer()
	{
		DEATH_ASSERT(vbo_ != nullptr);
		hasDirtyVertices_ = true;

#if defined(DEATH_TARGET_IOS)
		if (vboParams_.mapBase == nullptr) {
			vboParams_.mapBase = static_cast<std::uint8_t*>(vbo_->MapBufferRange(0, vbo_->GetSize(), 0));
		}
		return reinterpret_cast<float*>(vboParams_.mapBase);
#else
		if (vboParams_.mapBase == nullptr) {
			const GLenum mapFlags = RenderResources::GetBuffersManager().Specs(RenderBuffersManager::BufferTypes::Array).mapFlags;
			FATAL_ASSERT_MSG(mapFlags, "Mapping of OpenGL buffers is not available");
			vboParams_.mapBase = static_cast<std::uint8_t*>(vbo_->MapBufferRange(0, vbo_->GetSize(), mapFlags));
		}

		return reinterpret_cast<float*>(vboParams_.mapBase);
#endif
	}

	void Geometry::ReleaseVertexPointer()
	{
		// Don't flush and unmap if the VBO is not custom
		if (vbo_ != nullptr && vboParams_.mapBase != nullptr) {
			vboParams_.object->FlushMappedBufferRange(vboParams_.offset, vboParams_.size);
			vboParams_.object->Unmap();
		}
		vboParams_.mapBase = nullptr;
	}

	void Geometry::SetHostVertexPointer(const float* vertexPointer)
	{
		hasDirtyVertices_ = true;
		hostVertexPointer_ = vertexPointer;
	}

	void Geometry::ShareVbo(const Geometry* geometry)
	{
		if (geometry == nullptr) {
			sharedVboParams_ = nullptr;
		} else if (geometry != this) {
			vbo_.reset(nullptr);
			sharedVboParams_ = &geometry->vboParams_;
		}
	}

	void Geometry::CreateCustomIbo(std::uint32_t numIndices, BufferUsage usage)
	{
#if defined(DEATH_TARGET_IOS)
		ibo_ = std::make_unique<BackendBufferObject>(BackendBufferObject::Target::ElementArray);
		ibo_->BufferData(numIndices * sizeof(std::uint16_t), nullptr, 0);
#else
		ibo_ = std::make_unique<BackendBufferObject>(GL_ELEMENT_ARRAY_BUFFER);
		ibo_->BufferData(numIndices * sizeof(std::uint16_t), nullptr, GLMapping::BufferUsage(usage));
#endif

		iboUsageFlags_ = usage;
		iboParams_.object = ibo_.get();
		iboParams_.size = static_cast<std::uint32_t>(ibo_->GetSize());
		iboParams_.offset = 0;
		iboParams_.mapBase = nullptr;

#if defined(NCINE_PROFILING)
		RenderStatistics::AddCustomIbo(ibo_->GetSize());
#endif
	}

	std::uint16_t* Geometry::AcquireIndexPointer(std::uint32_t numIndices)
	{
		DEATH_ASSERT(ibo_ == nullptr);
		hasDirtyIndices_ = true;

#if defined(DEATH_TARGET_IOS)
		CreateCustomIbo(numIndices, BufferUsage::StreamDraw);
		iboParams_.mapBase = static_cast<std::uint8_t*>(ibo_->MapBufferRange(0, iboParams_.size, 0));
		return reinterpret_cast<std::uint16_t*>(iboParams_.mapBase);
#else
		if (sharedIboParams_ != nullptr) {
			iboParams_ = *sharedIboParams_;
		} else {
			const RenderBuffersManager::BufferTypes bufferType = RenderBuffersManager::BufferTypes::ElementArray;
			if (iboParams_.mapBase == nullptr) {
				iboParams_ = RenderResources::GetBuffersManager().AcquireMemory(bufferType, numIndices * sizeof(std::uint16_t));
			}
		}

		return reinterpret_cast<std::uint16_t*>(iboParams_.mapBase + iboParams_.offset);
#endif
	}

	/*! This method can only be used when mapping of OpenGL buffers is available */
	std::uint16_t* Geometry::AcquireIndexPointer()
	{
		DEATH_ASSERT(ibo_ != nullptr);
		hasDirtyIndices_ = true;

#if defined(DEATH_TARGET_IOS)
		if (iboParams_.mapBase == nullptr) {
			iboParams_.mapBase = static_cast<std::uint8_t*>(ibo_->MapBufferRange(0, ibo_->GetSize(), 0));
		}
		return reinterpret_cast<std::uint16_t*>(iboParams_.mapBase);
#else
		if (iboParams_.mapBase == nullptr) {
			const GLenum mapFlags = RenderResources::GetBuffersManager().Specs(RenderBuffersManager::BufferTypes::ElementArray).mapFlags;
			FATAL_ASSERT_MSG(mapFlags, "Mapping of OpenGL buffers is not available");
			iboParams_.mapBase = static_cast<std::uint8_t*>(ibo_->MapBufferRange(0, ibo_->GetSize(), mapFlags));
		}

		return reinterpret_cast<std::uint16_t*>(iboParams_.mapBase);
#endif
	}

	void Geometry::ReleaseIndexPointer()
	{
		// Don't flush and unmap if the IBO is not custom
		if (ibo_ != nullptr && iboParams_.mapBase != nullptr) {
			iboParams_.object->FlushMappedBufferRange(iboParams_.offset, iboParams_.size);
			iboParams_.object->Unmap();
		}
		iboParams_.mapBase = nullptr;
	}

	void Geometry::SetHostIndexPointer(const std::uint16_t* indexPointer)
	{
		hasDirtyIndices_ = true;
		hostIndexPointer_ = indexPointer;
	}

	void Geometry::ShareIbo(const Geometry* geometry)
	{
		if (geometry == nullptr) {
			sharedIboParams_ = nullptr;
		} else if (geometry != this) {
			ibo_.reset(nullptr);
			sharedIboParams_ = &geometry->iboParams_;
		}
	}

	void Geometry::Bind()
	{
#if defined(DEATH_TARGET_IOS)
		// No global bind state on Metal
		(void)0;
#else
		if (vboParams_.object != nullptr) {
			vboParams_.object->Bind();
		}
#endif
	}

	void Geometry::Draw(std::int32_t numInstances)
	{
#if defined(DEATH_TARGET_IOS)
		// Metal rendering is handled in RenderCommand::Issue()
		(void)numInstances;
		return;
#else
		const std::int32_t vboOffset = static_cast<std::int32_t>(GetVboParams().offset / numElementsPerVertex_ / sizeof(float)) + firstVertex_;

		void* iboOffsetPtr = nullptr;
		if (numIndices_ > 0) {
			iboOffsetPtr = reinterpret_cast<void*>(GetIboParams().offset + firstIndex_ * sizeof(std::uint16_t));
		}

		if (numInstances == 0) {
			if (numIndices_ > 0) {
#if (defined(WITH_OPENGLES) && !GL_ES_VERSION_3_2) || defined(DEATH_TARGET_EMSCRIPTEN)
				glDrawElements(GLMapping::PrimitiveType(primitiveType_), numIndices_, GL_UNSIGNED_SHORT, iboOffsetPtr);
#else
				glDrawElementsBaseVertex(GLMapping::PrimitiveType(primitiveType_), numIndices_, GL_UNSIGNED_SHORT, iboOffsetPtr, vboOffset);
#endif
			} else {
				glDrawArrays(GLMapping::PrimitiveType(primitiveType_), vboOffset, numVertices_);
			}
		} else if (numInstances > 0) {
			if (numIndices_ > 0) {
#if (defined(WITH_OPENGLES) && !GL_ES_VERSION_3_2) || defined(DEATH_TARGET_EMSCRIPTEN)
				glDrawElementsInstanced(GLMapping::PrimitiveType(primitiveType_), numIndices_, GL_UNSIGNED_SHORT, iboOffsetPtr, numInstances);
#else
				glDrawElementsInstancedBaseVertex(GLMapping::PrimitiveType(primitiveType_), numIndices_, GL_UNSIGNED_SHORT, iboOffsetPtr, numInstances, vboOffset);
#endif
			} else {
				glDrawArraysInstanced(GLMapping::PrimitiveType(primitiveType_), vboOffset, numVertices_, numInstances);
			}
		}
#endif
	}

	void Geometry::CommitVertices()
	{
		if (hostVertexPointer_ != nullptr && hasDirtyVertices_) {
			const std::uint32_t numFloats = numVertices_ * numElementsPerVertex_;

#if defined(DEATH_TARGET_IOS)
			// Always use a dedicated Metal buffer for now
			if (vbo_ == nullptr) {
				CreateCustomVbo(numFloats, BufferUsage::DynamicDraw);
			}
			vbo_->BufferData(numFloats * sizeof(float), hostVertexPointer_, 0);
			vboParams_.object = vbo_.get();
			vboParams_.size = static_cast<std::uint32_t>(vbo_->GetSize());
			vboParams_.offset = 0;
#else
			// Checking if the common VBO is allowed to use mapping and do the same for the custom one
			const GLenum mapFlags = RenderResources::GetBuffersManager().Specs(RenderBuffersManager::BufferTypes::Array).mapFlags;
			if (mapFlags == 0 && vbo_ != nullptr) {
				// Using buffer orphaning + `glBufferSubData()` when having a custom VBO with no mapping available
				vbo_->BufferData(vboParams_.size, nullptr, GLMapping::BufferUsage(vboUsageFlags_));
				vbo_->BufferSubData(vboParams_.offset, vboParams_.size, hostVertexPointer_);
			} else {
				float* vertices = vbo_ ? AcquireVertexPointer() : AcquireVertexPointer(numFloats, numElementsPerVertex_);
				memcpy(vertices, hostVertexPointer_, numFloats * sizeof(float));
				ReleaseVertexPointer();
			}
#endif

			// The dirty flag is only useful with a custom VBO. If the render command uses the common one, it must always copy vertices.
			if (vbo_ != nullptr) {
				hasDirtyVertices_ = false;
			}
		}
	}

	void Geometry::CommitIndices()
	{
		if (hostIndexPointer_ != nullptr && hasDirtyIndices_) {
#if defined(DEATH_TARGET_IOS)
			if (ibo_ == nullptr) {
				CreateCustomIbo(numIndices_, BufferUsage::DynamicDraw);
			}
			ibo_->BufferData(numIndices_ * sizeof(std::uint16_t), hostIndexPointer_, 0);
			iboParams_.object = ibo_.get();
			iboParams_.size = static_cast<std::uint32_t>(ibo_->GetSize());
			iboParams_.offset = 0;
#else
			// Checking if the common IBO is allowed to use mapping and do the same for the custom one
			const GLenum mapFlags = RenderResources::GetBuffersManager().Specs(RenderBuffersManager::BufferTypes::ElementArray).mapFlags;

			if (mapFlags == 0 && ibo_ != nullptr) {
				// Using buffer orphaning + `glBufferSubData()` when having a custom IBO with no mapping available
				ibo_->BufferData(iboParams_.size, nullptr, GLMapping::BufferUsage(iboUsageFlags_));
				ibo_->BufferSubData(iboParams_.offset, iboParams_.size, hostIndexPointer_);
			} else {
				std::uint16_t* indices = ibo_ ? AcquireIndexPointer() : AcquireIndexPointer(numIndices_);
				memcpy(indices, hostIndexPointer_, numIndices_ * sizeof(std::uint16_t));
				ReleaseIndexPointer();
			}
#endif

			// The dirty flag is only useful with a custom IBO. If the render command uses the common one, it must always copy indices.
			if (ibo_ != nullptr) {
				hasDirtyIndices_ = false;
			}
		}
	}
}
