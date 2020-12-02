// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here
#include "framework.h"

// Message Loop

#include <Windows.h>
#include <thread>

//WMFmisc

#include <tchar.h>
#include <evr.h>
#include <mfapi.h>
#include <mfplay.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <wmcodecdsp.h>
#include <codecapi.h>
#include <strsafe.h>

#include <mfobjects.h>
#include <mfcaptureengine.h>
#include <mfidl.h>
#include <shlwapi.h>

// Hardware camera control - brightness, focus, etc.

#include <strmif.h>
#pragma comment(lib, "Strmiids.lib")

// Device notification

#include <Dbt.h>
#include <ks.h>
#include <ksmedia.h>




#endif //PCH_H
