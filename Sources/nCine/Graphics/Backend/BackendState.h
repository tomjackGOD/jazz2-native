#pragma once

#include "../../Primitives/Colorf.h"
#include "BackendEnums.h"

namespace nCine
{
	struct ClearColorState
	{
		Colorf color;
	};

	struct ViewportState
	{
		int x, y, w, h;
	};

	struct ScissorState
	{
		bool enabled;
		int x, y, w, h;
	};
}
