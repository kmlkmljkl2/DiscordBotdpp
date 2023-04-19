/* Exit Games Common - C++ Client Lib
 * Copyright (C) 2004-2022 Exit Games GmbH. All rights reserved.
 * http://www.photonengine.com
 * mailto:developer@photonengine.com
 */

#pragma once

namespace ExitGames
{
	namespace Common
	{
		namespace Helpers
		{
			template<typename Ctype> struct IsFloatingPoint{static const bool is = false;};
			template<> struct IsFloatingPoint<float>{static const bool is = true;};
			template<> struct IsFloatingPoint<double>{static const bool is = true;};
			template<> struct IsFloatingPoint<long double>{static const bool is = true;};

			/** @file */
		}
	}
}