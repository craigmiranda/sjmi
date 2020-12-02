#pragma once

#include <locale>
#include <sstream>
#include <string>

// values must match java
#define FORMAT_NONE 0
#define FORMAT_ORIGINAL 0
#define FORMAT_RGB24 1
#define FORMAT_YUY2 2
#define FORMAT_MPG 3
#define FORMAT_NV12 4

#define PX_BYTES_RGB 3
#define PX_BYTES_YUY2 2

#define STOPPED 0
#define STOPPING 1
#define STREAMING 2

const int STREAM_OFF = 1000;
const int STREAM_ON = 2000;
const int PHOTO_TAKEN = 3000;
const int DEVICE_LOST = 4000;
const int DEVICE_NEW = 5000;
const int DEVICE_DEACTIVATED = 6000;
const int DEVICE_ACTIVATED = 7000;

const int ERROR_INIT = 0;
const int ERROR_STREAM = 1000;
const int ERROR_STREAM_DEVICE_FIRST = 1001;
const int ERROR_STREAM_DEVICE_MID = 1002;
const int ERROR_STILL = 3000;
const int ERROR_DEVICE = 4000;
const int ERROR_DEACTIVATE = 6000;
const int ERROR_ACTIVATE = 7000;
const int ERROR_UNINIT = 8000;

struct JVM
{
    JavaVM* vm = NULL;
    int version = 0;
};

struct DeviceAttr // container for GUID attr/value
{
    GUID guid;
    PROPVARIANT var;
};

struct CaptureFormat
{
    DeviceAttr* captureFormatAttr; 
    UINT32 attrCount;
};

struct Device
{
    int id{-1};
    std::string friendlyName;
    CaptureFormat* capFormat = NULL;
    UINT32 capFormatCount{ 0 };

    // for device change notifications
    WCHAR* g_pwszSymbolicLink = NULL;
    UINT32 g_cchSymbolicLink = 0;

};

struct ActiveDevice
{
    Device device;
    
    IMFMediaSource* pVideoSource;
    IMFMediaType* pVideoMediaType;
    IMFMediaType* pPhotoMediaType;
    CaptureFormat captureFormat;

    short streaming{STOPPED};
    jobject rcvr;

    thread th;
};


PROPVARIANT GetAttrVarByGUID(const GUID& guid, CaptureFormat& cf) 
{

    int i = 0;

    while (i < cf.attrCount)
    {
        if (guid == cf.captureFormatAttr[i].guid)
            return cf.captureFormatAttr[i].var;

        i++;
    }

    PROPVARIANT var;
    PropVariantInit(&var); // TODO: return a VT_NULL; not sure how
    return var;
}


// forward decl.

HRESULT GetAvailDevices();


// odds and sods functions

std::string ToNarrow(const wchar_t* s, char dfault = '?',
    const std::locale& loc = std::locale())
{
    std::ostringstream stm;

    while (*s != L'\0') {
        stm << std::use_facet< std::ctype<wchar_t> >(loc).narrow(*s++, dfault);
    }
    return stm.str();
}


