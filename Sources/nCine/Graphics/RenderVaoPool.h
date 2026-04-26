#pragma once

#include "../Base/TimeStamp.h"
#include "GL/GLVertexArrayObject.h"
#include "GL/GLVertexFormat.h"

#include <memory>

#if defined(__has_include)
#	if __has_include("Shared/Containers/SmallVector.h")
#		include "Shared/Containers/SmallVector.h"
#	elif __has_include("../Shared/Containers/SmallVector.h")
#		include "../Shared/Containers/SmallVector.h"
#	elif __has_include("../../Shared/Containers/SmallVector.h")
#		include "../../Shared/Containers/SmallVector.h"
#	else
#		include <Containers/SmallVector.h>
#	endif
#else
#	include <Containers/SmallVector.h>
#endif

using namespace Death::Containers;

namespace nCine
{
	class GLVertexArrayObject;

	/// Creates and handles the pool of VAOs
	class RenderVaoPool
	{
	public:
		explicit RenderVaoPool(std::uint32_t vaoPoolSize);

		void BindVao(const GLVertexFormat& vertexFormat);

	private:
#ifndef DOXYGEN_GENERATING_OUTPUT
		// Doxygen 1.12.0 outputs also private structs/unions even if it shouldn't
		struct VaoBinding
		{
			std::unique_ptr<GLVertexArrayObject> object;
			GLVertexFormat format;
			TimeStamp lastBindTime;
		};
#endif

		SmallVector<VaoBinding, 0> vaoPool_;

		void InsertGLDebugMessage(const VaoBinding& binding);
	};

}
