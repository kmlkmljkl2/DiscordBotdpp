/* Exit Games Common - C++ Client Lib
 * Copyright (C) 2004-2023 Exit Games GmbH. All rights reserved.
 * http://www.photonengine.com
 * mailto:developer@photonengine.com
 */

#pragma once

#include "AoTTG/Common-cpp/inc/Helpers/TypeTraits/IsArithmetic.h"
#include "AoTTG/Common-cpp/inc/Helpers/TypeTraits/IsVoid.h"

namespace ExitGames
{
	namespace Common
	{
		namespace Helpers
		{
			template<typename Ctype> struct IsFundamental{static const bool is = IsVoid<Ctype>::is || IsArithmetic<Ctype>::is;};

			/** @file */
		}
	}
}