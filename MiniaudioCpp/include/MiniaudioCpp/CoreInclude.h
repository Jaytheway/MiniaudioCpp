//
//      ██╗██████╗     ██╗     ██╗██████╗ ███████╗
//      ██║██╔══██╗    ██║     ██║██╔══██╗██╔════╝		** MiniaudioCpp **
//      ██║██████╔╝    ██║     ██║██████╔╝███████╗
// ██   ██║██╔═══╝     ██║     ██║██╔══██╗╚════██║		https://github.com/Jaytheway/MiniaudioCpp
// ╚█████╔╝██║         ███████╗██║██████╔╝███████║
//  ╚════╝ ╚═╝         ╚══════╝╚═╝╚═════╝ ╚══════╝
//
//   Copyright 2026 Jaroslav Pevno, MiniaudioCpp is offered under the terms of the ISC license:
//
//   Permission to use, copy, modify, and/or distribute this software for any purpose with or
//   without fee is hereby granted, provided that the above copyright notice and this permission
//   notice appear in all copies. THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//   WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
//   AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
//   CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
//   WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
//   CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#pragma once

// MiniaudioCpp and JPL Spatial use pretty much the same Core.h and ErrorReporting.h
// When both libraries are used in the same project, only one verstion of the files should be used,
// JPL Spatial's files should take precedence and the following macros shuold be set
// by the build system accordingly.

#if (defined(JPL_CORE_INCLUDE) and !defined(JPL_ERROR_REPORTING_INCLUDE)) or (!defined(JPL_CORE_INCLUDE) and defined(JPL_ERROR_REPORTING_INCLUDE))
#Error "JPL_CORE_INCLUDE and JPL_ERROR_REPORTING_INCLUDE: Either both or none must be overriden."
#endif

#ifdef JPL_CORE_INCLUDE
	#include JPL_CORE_INCLUDE
#else
	#include "Core.h"
#endif

#ifdef JPL_ERROR_REPORTING_INCLUDE
	#include JPL_ERROR_REPORTING_INCLUDE
#else
	#include "ErrorReporting.h"
#endif
