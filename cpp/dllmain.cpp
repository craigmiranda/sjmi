#include "pch.h"

#include <stdio.h>
#include <iostream>
#include <cassert>
#include <cwchar>
#include <clocale>
#include <chrono>

using namespace std;

//#include <jni.h>
#include "com_rockenbrew_JavaWMF_SJMI.h" // JNI headers
#include "com_rockenbrew_JavaWMF_VideoDevice.h" // Device specifc JNI headers
#include "misc.h" // odds and sods
#include "WMFmisc.h" // Misc. Windows Media Foundation functions (only nominally modified) from https://docs.microsoft.com/en-us/windows/win32/api/_mf/  
#include "Transform.h"

// Still photo's require Capture Engine DLL
boolean mfCaptureEngineDLLAvail;

typedef HRESULT(__stdcall* f_MFCreateCaptureEngine)(IMFCaptureEngine**);

f_MFCreateCaptureEngine MFCreateCaptureEngine;

const UINT WM_APP_CAPTURE_EVENT = WM_APP + 1;

ActiveDevice* activeDevice;
int activeDeviceIndex = -1; // TODO: allow multiple live devices
int activeDeviceCount = 0; // camera device index

Device* availDevice;
UINT32 availDeviceCount = 0;

jobject globalRcvr;

boolean wait;

JVM jvm;

boolean init = false;

#include "MessageLoop.h" // 

boolean Deactivate(int deviceIndex);
boolean StopStream();

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    if (!init && hModuleG == NULL)
    {
        init = true;

        hModuleG = hModule;

    }
    else
        return FALSE;

    return TRUE;
}


void JavaNotifyError(int errorCode, string msg) {

    const char* streamName = "NotifyError";

    JNIEnv* env;
    JavaVMAttachArgs args;
    args.version = jvm.version;
    args.name = (char*)streamName;
    args.group = NULL;
    jvm.vm->AttachCurrentThread((void**)&env, &args);

    jobject& rcvr = globalRcvr;

    // Find the id of the Java method to be called
    jclass rcvrClass = env->GetObjectClass(rcvr);
    jmethodID rcvrNotifyErrorMethod = env->GetMethodID(rcvrClass, "notifyError", "(ILjava/lang/String;)V");

    jstring jErrMsg = env->NewStringUTF(msg.c_str());

    env->CallObjectMethod(globalRcvr, rcvrNotifyErrorMethod, errorCode, jErrMsg);

    jvm.vm->DetachCurrentThread();
}


void JavaNotifyEvent(int eventCode) {

    const char* streamName = "NotifyEvent";

    JNIEnv* env;
    JavaVMAttachArgs args;
    args.version = jvm.version;
    args.name = (char*)streamName;
    args.group = NULL;
    jvm.vm->AttachCurrentThread((void**)&env, &args);

    jobject& rcvr = globalRcvr;

    // Find the id of the Java method to be called
    jclass rcvrClass = env->GetObjectClass(rcvr);
    jmethodID rcvrNotifyEventMethod = env->GetMethodID(rcvrClass, "notifyEvent", "(I)V");

    env->CallObjectMethod(globalRcvr, rcvrNotifyEventMethod, eventCode);

    jvm.vm->DetachCurrentThread();
}


boolean LoadMFCaptureEngineDLL() {

    // load DLL
    HMODULE MFCaptureEngineDLL = LoadLibraryA("MFCaptureEngine.dll");
    if (!MFCaptureEngineDLL)
        return FALSE;

    // find/link functions
    MFCreateCaptureEngine = (f_MFCreateCaptureEngine)GetProcAddress(MFCaptureEngineDLL, "MFCreateCaptureEngine");
    if (!MFCreateCaptureEngine)
        return FALSE;

    return TRUE;
}

HRESULT EnumerateVideoDevices(IMFAttributes*& pAttributes, IMFActivate**& ppDevices, UINT32& count) {

    // return available devices (as IMFActivate objects)
    // This function may be used as a step to list devices and/or activate a device by other functions

    // Create an attribute store to specify the enumeration parameters.
    HRESULT hr = MFCreateAttributes(&pAttributes, 1);
    if (FAILED(hr))
    {
        //goto done;
    }

    // Source type: video capture devices
    hr = pAttributes->SetGUID(
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
    );
    if (FAILED(hr))
    {
        //goto done;
    }

    // Enumerate devices.
    hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);
    if (FAILED(hr))
    {
        //goto done;
    }

    if (count == 0)
    {
        hr = E_FAIL;
        //goto done;
    }

    return hr;
}

JNIEXPORT jobjectArray JNICALL Java_com_rockenbrew_JavaWMF_SJMI_GetAvailDevices
(JNIEnv* env, jobject thisObject) {
    // return pair of "device friendly name", "device symbolic link" for each device

    HRESULT hr = GetAvailDevices();

    jclass stringClass = env->FindClass("java/lang/String");
    jobjectArray array = env->NewObjectArray(availDeviceCount * 2, stringClass, 0);

    for (jsize i = 0; i < availDeviceCount; i++)
    {

        jstring deviceFriendlyName = env->NewStringUTF(availDevice[i].friendlyName.c_str());
        env->SetObjectArrayElement(array, i * 2, deviceFriendlyName);

        jstring deviceSymbolicLink = env->NewStringUTF(ToNarrow(availDevice[i].g_pwszSymbolicLink).c_str());
        env->SetObjectArrayElement(array, i * 2 + 1, deviceSymbolicLink);
    }

    return array;
}

HRESULT GetAvailDevices()
{

    IMFAttributes* pAttributes = NULL;
    IMFActivate** ppDevices = NULL;

    HRESULT hr = EnumerateVideoDevices(pAttributes, ppDevices, availDeviceCount);

    availDevice = new Device[availDeviceCount]; // initialise empty return array

    // populate return array w/ devices
    for (int i = 0; i < availDeviceCount; i++)
    {
        wchar_t* deviceFriendlyName;
        UINT len = 0;

        hr = ppDevices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &deviceFriendlyName, &len);
        Device newDevice{ i, ToNarrow(deviceFriendlyName), NULL, 0 }; // TODO: check that change from unicode to String doesn't screw anything up (see ToNarrow, misc.h)

        // retain for device change notificatios
        ppDevices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
            &newDevice.g_pwszSymbolicLink, &newDevice.g_cchSymbolicLink);

        availDevice[i] = newDevice;
    }

done:
    SafeRelease(&pAttributes);

    for (DWORD i = 0; i < availDeviceCount; i++)
    {
        SafeRelease(&ppDevices[i]);
    }
    CoTaskMemFree(ppDevices);
    return hr;
}


HRESULT CheckVsAvailDevices(DEV_BROADCAST_HDR* pHdr)
{

    DEV_BROADCAST_DEVICEINTERFACE* pDi = NULL;

    if (activeDeviceIndex >= 0 && activeDevice[activeDeviceIndex].pVideoSource == NULL)
    {
        return S_OK;
    }
    if (pHdr == NULL)
    {
        return S_OK;
    }
    if (pHdr->dbch_devicetype != DBT_DEVTYP_DEVICEINTERFACE)
    {
        return S_OK;
    }

    string str;

    // Get symbolic link id of affected device.

    pDi = (DEV_BROADCAST_DEVICEINTERFACE*)pHdr;
    WCHAR* affectDeviceSymbolicLink = pDi->dbcc_name;


    int i = 0;

    // test for device lost (ie. affected device was in the avilable list)
    while (i < availDeviceCount)
    {
        if (_wcsnicmp(affectDeviceSymbolicLink, availDevice[i].g_pwszSymbolicLink, 50) == 0) // compare only the first 50 chars; TODO: find out why the remainder vary (maybe threading?)
        {
            JavaNotifyEvent(DEVICE_LOST + i);
            break;
        }

        i++;
    }

    // if affect device was not in the available list, presume this is a new device
    if (i == availDeviceCount)
    {
        JavaNotifyEvent(DEVICE_NEW + i);
    }
    else
    {

        // shutdown the active device, if lost 
        for (int j = 0; j < activeDeviceCount; j++)
            if (_wcsnicmp(affectDeviceSymbolicLink, activeDevice[j].device.g_pwszSymbolicLink, 50) == 0) // compare only the first 50 chars; TODO: find out why the remainder vary (maybe threading?)
            {

                Deactivate(j);

                break;
            }
    }

    // re-initialise devices; this doesn't affect the java handler
    GetAvailDevices();

    return S_OK;
}


HRESULT CreateVideoDeviceSource(IMFMediaSource** ppSource, int deviceIndex)
{
    *ppSource = NULL;

    IMFMediaSource* pSource = NULL;
    IMFAttributes* pAttributes = NULL;
    IMFActivate** ppDevices = NULL;

    UINT32 count;
    HRESULT hr = EnumerateVideoDevices(pAttributes, ppDevices, count);

    if (deviceIndex >= 0 && deviceIndex < count) // a device in range exists
    {

        // Create the media source object.
        hr = ppDevices[deviceIndex]->ActivateObject(IID_PPV_ARGS(&pSource));
        if (FAILED(hr))
        {
            printf("unable to activate source\n");
            goto done;
        }

        *ppSource = pSource;
        (*ppSource)->AddRef();

    }
    else
    {
        hr = E_FAIL; // generic failure
    }

done:
    SafeRelease(&pAttributes);

    for (DWORD i = 0; i < count; i++)
    {
        SafeRelease(&ppDevices[i]);
    }
    CoTaskMemFree(ppDevices);
    SafeRelease(&pSource);

    return hr;
}


HRESULT LogMediaType(IMFMediaType* pType, CaptureFormat& capFormat)
{
    UINT32& count = capFormat.attrCount;

    HRESULT hr = pType->GetCount(&count);
    if (FAILED(hr))
    {
        return hr;
    }

    if (count == 0)
    {
        wprintf(L"Empty media type.\n");
    }

    capFormat.captureFormatAttr = new DeviceAttr[count]; // initialise capture format with number of avail. attrerties    

    // for each media type (captureFormatAttr)
    for (UINT32 i = 0; i < count; i++)
    {

        hr = LogAttributeValueByIndex(pType, i, capFormat.captureFormatAttr[i]);
        if (FAILED(hr))
        {
            break;
        }

    }

    return hr;
}

HRESULT LogAttributeValueByIndex(IMFAttributes* pAttr, DWORD index, DeviceAttr& attr) // CM modified to store (raw) GUID & PROPVARIANT for easier access later
{
    GUID guid = { 0 };
    PROPVARIANT var;
    PropVariantInit(&var);

    HRESULT hr = pAttr->GetItemByIndex(index, &guid, &var);
    if (FAILED(hr))
        return hr;

    attr.guid = guid;
    attr.var = var;
}

HRESULT GetAttrStrings(DeviceAttr& attr, std::string& name, std::string& value) // CM modified to read string values; Note. potential issues where I've translated wide char (WCHAR) to narrow char std::strings.
{
    WCHAR* pGuidName = NULL;
    WCHAR* pGuidValName = NULL;

    string specialAttrStr;
    WCHAR* pGUIDname = NULL;

    GUID& guid = attr.guid;
    PROPVARIANT& var = attr.var;

    HRESULT hr = GetGUIDName(guid, &pGuidName);
    if (FAILED(hr))
    {
        goto done;
    }

    name = ToNarrow(pGuidName);

    hr = SpecialCaseAttributeValue(guid, var, specialAttrStr); // Special cases, including frame, frame rate, etc. ("I x J"-type dimensions) see WMFmisc.h (CM); CM edited this and sub functions (see WMFmisc.h)
    if (FAILED(hr))
    {
        goto done;
    }
    if (hr == S_FALSE) // not "special case"
    {

        switch (var.vt) // vt=variable type (see https://docs.microsoft.com/en-us/windows/win32/api/propidlbase/ns-propidlbase-propvariant)
        {
        case VT_UI4:
            //DBGMSG(L"%d", var.ulVal);
            value = MSGtoString(L"%d", var.ulVal);
            break;

        case VT_UI8:
            //DBGMSG(L"%I64d", var.uhVal);
            value = MSGtoString(L"%I64d", var.uhVal);
            break;

        case VT_R8:
            //DBGMSG(L"%f", var.dblVal);
            value = MSGtoString(L"%f", var.dblVal);
            break;

        case VT_CLSID:
            hr = GetGUIDName(*var.puuid, &pGuidValName);
            if (SUCCEEDED(hr))
            {
                value = MSGtoString(pGuidValName);
            }
            break;

        case VT_LPWSTR:
            //DBGMSG(var.pwszVal);
            value = MSGtoString(var.pwszVal);
            break;

        case VT_VECTOR | VT_UI1:
            //DBGMSG(L"<<byte array>>");
            value = "<<byte array>>";
            break;

        case VT_UNKNOWN:
            //DBGMSG(L"IUnknown");
            value = "IUnknown";
            break;

        default:
            //DBGMSG(L"Unexpected attribute type (vt = %d)", var.vt);
            value = MSGtoString(L"Unexpected attribute type (vt = %d)", var.vt);
            break;
        }
    }
    else
    {
        value = specialAttrStr;
    }


done:
    CoTaskMemFree(pGuidName);
    CoTaskMemFree(pGuidValName);
    //PropVariantClear(&var);
    return hr;
}


HRESULT GetMediaTypeHandler(IMFMediaSource*& pSource, IMFMediaTypeHandler** ppHandler) // CM split out from EnumerateCaptureFormats function
{

    IMFPresentationDescriptor* pPD = NULL;
    IMFStreamDescriptor* pSD = NULL;
    IMFMediaTypeHandler* pHandler = NULL;

    HRESULT hr = pSource->CreatePresentationDescriptor(&pPD);
    if (FAILED(hr))
    {
        goto done;
    }

    BOOL fSelected;
    hr = pPD->GetStreamDescriptorByIndex(0, &fSelected, &pSD);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pSD->GetMediaTypeHandler(&pHandler);
    if (FAILED(hr))
    {
        goto done;
    }

    DWORD cTypes;
    hr = pHandler->GetMediaTypeCount(&cTypes);
    if (FAILED(hr))
    {
        goto done;
    }

    *ppHandler = pHandler;
    (*ppHandler)->AddRef();

done:
    SafeRelease(&pPD);
    SafeRelease(&pSD);
    SafeRelease(&pHandler);
    return hr;
}

HRESULT EnumerateCaptureFormats(IMFMediaSource* pSource, CaptureFormat*& capFormat, UINT32& cfCount) // CM added parameter ", CaptureFormat*& capFormat"
{

    //IMFPresentationDescriptor* pPD = NULL;
    //IMFStreamDescriptor* pSD = NULL;
    IMFMediaTypeHandler* pHandler = NULL;
    IMFMediaType* pType = NULL;

    //HRESULT hr = pSource->CreatePresentationDescriptor(&pPD);
    //if (FAILED(hr))
    //{
    //    goto done;
    //}

    //BOOL fSelected;
    //hr = pPD->GetStreamDescriptorByIndex(0, &fSelected, &pSD);
    //if (FAILED(hr))
    //{
    //    goto done;
    //}

    //hr = pSD->GetMediaTypeHandler(&pHandler);
    //if (FAILED(hr))
    //{
    //    goto done;
    //}

    GetMediaTypeHandler(pSource, &pHandler);

    DWORD cTypes;
    HRESULT hr = pHandler->GetMediaTypeCount(&cTypes);
    if (FAILED(hr))
    {
        goto done;
    }

    capFormat = new CaptureFormat[cTypes];
    cfCount = static_cast<UINT32>(cTypes);

    // For each CaptureFormat, get properties
    for (DWORD i = 0; i < cTypes; i++)
    {
        hr = pHandler->GetMediaTypeByIndex(i, &pType);
        if (FAILED(hr))
        {
            goto done;
        }

        LogMediaType(pType, capFormat[i]); // CM added argument capFormat[i]
        SafeRelease(&pType);
    }

done:
    SafeRelease(&pHandler);
    SafeRelease(&pType);
    return hr;
}


JNIEXPORT jboolean JNICALL Java_com_rockenbrew_JavaWMF_SJMI_Initialise
(JNIEnv* env, jobject thisObject)
{

    mfCaptureEngineDLLAvail = LoadMFCaptureEngineDLL();

    // intialise VM for threads
    jvm = JVM{ NULL,NULL };
    env->GetJavaVM(&jvm.vm);
    jvm.version = env->GetVersion();

    // Initialise the COM runtime.
    HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (SUCCEEDED(hr)) {

        // Initialise the Media Foundation platform.
        hr = MFStartup(MF_VERSION);
        if (SUCCEEDED(hr))
        {

            GetAvailDevices();
            return true;
        }
    }

    return false;

}

JNIEXPORT jboolean JNICALL Java_com_rockenbrew_JavaWMF_SJMI_Monitor
(JNIEnv* env, jobject thisObject, jobject rcvr)
{

    globalRcvr = rcvr;

    std::thread th2(initMessageLoop);
    th2.join();

    return TRUE;
}

JNIEXPORT jboolean JNICALL Java_com_rockenbrew_JavaWMF_SJMI_Uninitialise
(JNIEnv* env, jobject thisObject)
{
    return Uninitialise();
}

boolean Uninitialise()
{

    for (int i = 0; i < activeDeviceCount; i++)
    {
        Deactivate(i);
    }

    // Uninitialize the COM runtime.
    HRESULT hr = MFShutdown();
    CoUninitialize();

    if (SUCCEEDED(hr))
        return true;
    else
        return false; // note. we assume couninitialize worked

}


JNIEXPORT jboolean JNICALL Java_com_rockenbrew_JavaWMF_VideoDevice_Activate
(JNIEnv* env, jobject thisObject, jint deviceIndex)
{

    GetAvailDevices(); // (re)load device list to ensure device index exists
    if (deviceIndex < 0 && deviceIndex >= availDeviceCount) // TODO: and not already active
    {
        JavaNotifyError(1, "Err: Invalid device ID");
        return false;
    }

    activeDevice = new ActiveDevice{ availDevice[deviceIndex], NULL, NULL, NULL }; // ActiveDevice includes WMF objects to operate that device. TODO: expand to allow multiple devices
    activeDeviceIndex = 0; // index of active device (relevant if using multiple active devices)

    ActiveDevice& actDevice = activeDevice[activeDeviceIndex];

    HRESULT hr = CreateVideoDeviceSource(&actDevice.pVideoSource, deviceIndex); // set device
    if (FAILED(hr))
    {
        JavaNotifyError(1, "Err: CreateVideoDeviceSource");
        goto done;
    }

    activeDeviceCount++;
    return true;

done:
    return false;
}

JNIEXPORT jboolean JNICALL Java_com_rockenbrew_JavaWMF_VideoDevice_Deactivate
(JNIEnv* env, jobject thisObject, jint deviceIndex)
{

    GetAvailDevices(); // (re)load device list to ensure device index exists
    if (deviceIndex < 0 && deviceIndex >= availDeviceCount) // TODO: and not already active
        return false; // invalid device index

    return Deactivate(activeDeviceIndex); // TODO: mutli-device

}

boolean Deactivate(int deviceIndex) {

    if (activeDeviceCount < 1)
        return false;

    if (activeDevice[deviceIndex].streaming)
        StopStream();

    SafeRelease(&activeDevice[deviceIndex].pVideoMediaType);
    SafeRelease(&activeDevice[deviceIndex].pPhotoMediaType);

    IMFMediaSource*& pVideoSource = activeDevice[deviceIndex].pVideoSource;

    if (pVideoSource)
    {
        HRESULT hr = pVideoSource->Shutdown();
        if (SUCCEEDED(hr))
        {
            pVideoSource->Release();
            pVideoSource = NULL;
        }
    }

    SafeRelease(&activeDevice[deviceIndex].pVideoSource);

    JavaNotifyEvent(DEVICE_DEACTIVATED + activeDevice[deviceIndex].device.id);

    activeDevice[deviceIndex] = ActiveDevice{};
    activeDeviceCount--;
    activeDeviceIndex = -1;

    return true;

}

JNIEXPORT jobjectArray JNICALL Java_com_rockenbrew_JavaWMF_VideoDevice_GetRawAvailCaptureFormat
(JNIEnv* env, jobject thisObject)
{
    // return an 2d string array of captureFormat & CaptureFormatAttr as a sequence of strings

    CaptureFormat*& capFormat = activeDevice[activeDeviceIndex].device.capFormat;
    UINT32& cpCount = activeDevice[activeDeviceIndex].device.capFormatCount;

    HRESULT hr = EnumerateCaptureFormats(activeDevice[activeDeviceIndex].pVideoSource, capFormat, cpCount);

    // 2d array
    jclass stringClass = env->FindClass("java/lang/String"); // inner array
    jobjectArray stringStringClass = env->NewObjectArray(1, stringClass, 0); // necessary to define 2d string object type
    jobjectArray CapFormatArray = env->NewObjectArray(cpCount, env->GetObjectClass(stringStringClass), 0); // outer array of string array

    for (int i = 0; i < cpCount; i++) // for each available Capture Format
    {

        int pCount = capFormat[i].attrCount;
        CaptureFormat& capF = capFormat[i];

        jsize attrCount = pCount * 2; // num of strings to return to java
        jobjectArray AttrArray = env->NewObjectArray(attrCount, stringClass, 0);

        for (int j = 0; j < pCount; j++) // for each attribute (of each Capture Format)
        {

            string name, value;
            GetAttrStrings(capF.captureFormatAttr[j], name, value); // get strings from GUID & PROPVARIANT

            jstring nameStr = env->NewStringUTF(name.c_str());
            jstring valueStr = env->NewStringUTF(value.c_str());
            env->SetObjectArrayElement(AttrArray, j * 2, nameStr);
            env->SetObjectArrayElement(AttrArray, j * 2 + 1, valueStr);
        }

        env->SetObjectArrayElement(CapFormatArray, i, AttrArray);
    }
    return CapFormatArray;

}


JNIEXPORT jboolean JNICALL Java_com_rockenbrew_JavaWMF_VideoDevice_SetActiveCaptureFormat
(JNIEnv* env, jobject thisObject, jint captureFormatIndex) {

    // check valid capFormatIndex
    UINT32 capFormatCount = activeDevice[activeDeviceIndex].device.capFormatCount;
    if (capFormatCount < 0 || captureFormatIndex >= capFormatCount)
    {
        JavaNotifyError(1, "Invalid capture format index");
        return false;
    }

    ActiveDevice& actDevice = activeDevice[activeDeviceIndex];
    CaptureFormat& capFormat = actDevice.device.capFormat[captureFormatIndex];

    //// Get Video Format GUID
    //PROPVARIANT stVar = GetAttrVarByGUID(MF_MT_SUBTYPE, capFormat);
    //GUID videoFormatGUID = *stVar.puuid;

    //// Get Frame Rate of given CaptureFormat
    //UINT32 frameRate=30, frameRateDenominator=0; // eg. for "30x1" frameRate = 30, frameRate2 = 1
    //PROPVARIANT fVar = GetAttrVarByGUID(MF_MT_FRAME_RATE, capFormat);
    //LogUINT32AsUINT64(fVar, frameRate, frameRateDenominator);

    //// Get Frame Rate of given CaptureFormat
    //UINT32 frameWidth=640, frameHeight=480;
    //LogUINT32AsUINT64(GetAttrVarByGUID(MF_MT_FRAME_SIZE, capFormat), frameWidth, frameHeight);

    //// set up with given CaptureFormat details
    //MFCreateMediaType(&actDevice.pMediaType);
    //actDevice.pMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video); // video only; TODO: audio?
    //actDevice.pMediaType->SetGUID(MF_MT_SUBTYPE, videoFormatGUID);
    //MFSetAttributeRatio(actDevice.pMediaType, MF_MT_FRAME_RATE, frameRate, frameRateDenominator);
    //MFSetAttributeSize(actDevice.pMediaType, MF_MT_FRAME_SIZE, frameWidth, frameHeight

        // Set Media Type by Index
    IMFMediaTypeHandler* mediaTypeHandler = NULL;
    GetMediaTypeHandler(actDevice.pVideoSource, &mediaTypeHandler);
    mediaTypeHandler->GetMediaTypeByIndex(captureFormatIndex, &actDevice.pVideoMediaType);

    actDevice.captureFormat = capFormat; // active capture format

    HRESULT hr = MFTRegisterLocalByCLSID(
        __uuidof(CColorConvertDMO),
        MFT_CATEGORY_VIDEO_PROCESSOR,
        L"",
        MFT_ENUM_FLAG_SYNCMFT,
        0,
        NULL,
        0,
        NULL
    );
    if (FAILED(hr))
        return false;

    return true;
}


JNIEXPORT jboolean JNICALL Java_com_rockenbrew_JavaWMF_VideoDevice_SetVideoProp
(JNIEnv* env, jobject thisObject, jlong videoProp, jlong value)
{

    // settings: https://docs.microsoft.com/en-us/windows/win32/api/strmif/ne-strmif-videoprocampproperty

    ActiveDevice& actDevice = activeDevice[activeDeviceIndex];
    IMFMediaSource*& activeDeviceSource = actDevice.pVideoSource;

    //// Query the capture filter for the IAMVideoProcAmp interface.
    IAMVideoProcAmp* pProcAmp = NULL;
    HRESULT hr = activeDeviceSource->QueryInterface(IID_IAMVideoProcAmp, (void**)&pProcAmp);
    if (FAILED(hr))
        goto doneErr;

    //long videoProp =
        //VideoProcAmp_Brightness
        //VideoProcAmp_Contrast
        //VideoProcAmp_Hue
        //VideoProcAmp_Saturation
        //VideoProcAmp_Sharpness
        //VideoProcAmp_Gamma
        //VideoProcAmp_ColorEnable
        //VideoProcAmp_WhiteBalance
        //VideoProcAmp_BacklightCompensation
        //VideoProcAmp_Gain
        //;

    hr = pProcAmp->Set(videoProp, value, 0); // set new value
    if (SUCCEEDED(hr))
    {
        SafeRelease(&pProcAmp);
        return true;
    }

doneErr:

    //JavaNotifyError(1, "Err: Unable to change video property");
    SafeRelease(&pProcAmp);
    return false;
}


const int PROP_VALUE = 0;
const int PROP_MIN = 1;
const int PROP_MAX = 2;
const int PROP_STEP = 3;
const int PROP_DEFAULT = 4;
const int PROP_AUTO_FLAG = 5;

const jlong ERROR_VALUE_JLONG = -9223372036854775807L;


JNIEXPORT jlong JNICALL Java_com_rockenbrew_JavaWMF_VideoDevice_GetVideoProp
(JNIEnv* env, jobject thisObject, jlong videoProp, jint type)
{

    // settings: https://docs.microsoft.com/en-us/windows/win32/api/strmif/ne-strmif-videoprocampproperty

    ActiveDevice& actDevice = activeDevice[activeDeviceIndex];
    IMFMediaSource*& activeDeviceSource = actDevice.pVideoSource;

    //// Query the capture filter for the IAMVideoProcAmp interface.
    IAMVideoProcAmp* pProcAmp = NULL;
    HRESULT hr = activeDeviceSource->QueryInterface(IID_IAMVideoProcAmp, (void**)&pProcAmp);
    if (FAILED(hr))
        goto doneErr;

    long min, max, step, defaultVal, autoFlag, val; // Auto=1L/Manual=2L/https://docs.microsoft.com/en-us/windows/win32/api/strmif/ne-strmif-cameracontrolflags

    switch (type) {

    case PROP_VALUE:
    case PROP_AUTO_FLAG:

        hr = pProcAmp->Get(videoProp, &val, &autoFlag);
        if (SUCCEEDED(hr))
        {

            SafeRelease(&pProcAmp);

            switch (type) {
            case PROP_VALUE: return static_cast<jlong>(val);
            case PROP_AUTO_FLAG: return static_cast<jlong>(autoFlag);
            }
        }
        else
            goto doneErr;

    default:

        hr = pProcAmp->GetRange(videoProp, &min, &max, &step, &defaultVal, &autoFlag);
        if (SUCCEEDED(hr))
        {

            SafeRelease(&pProcAmp);

            switch (type) {
            case PROP_MIN: return static_cast<jlong>(min);
            case PROP_MAX: return static_cast<jlong>(max);
            case PROP_STEP: return static_cast<jlong>(step);
            case PROP_DEFAULT: return static_cast<jlong>(defaultVal);
            }
        }
        else
            goto doneErr;

        break;
    }


doneErr:

    //JavaNotifyError(1, "Err: Unable to get video property");
    SafeRelease(&pProcAmp);
    return ERROR_VALUE_JLONG;
}


JNIEXPORT jboolean JNICALL Java_com_rockenbrew_JavaWMF_VideoDevice_SetDeviceProp
(JNIEnv* env, jobject thisObject, jlong cameraProp, jlong value)
{

    // more settings; exposure, focus, etc...
// https://docs.microsoft.com/en-us/windows/win32/api/strmif/ne-strmif-cameracontrolproperty

    ActiveDevice& actDevice = activeDevice[activeDeviceIndex];
    IMFMediaSource*& activeDeviceSource = actDevice.pVideoSource;

    // Query the capture filter for the IAMVideoProcAmp interface.
    IAMCameraControl* pCamControl = 0;
    HRESULT hr = activeDeviceSource->QueryInterface(IID_IAMCameraControl, (void**)&pCamControl);
    if (FAILED(hr))
        goto doneErr;

    hr = pCamControl->Set(cameraProp, value, CameraControl_Flags_Manual); // set new value; exposure: For values less than zero, the exposure time is 1/(2n) seconds. For values of zero and above, the exposure time is 2n seconds.
    if (SUCCEEDED(hr))
    {
        SafeRelease(&pCamControl);
        return true;
    }

doneErr:

    //JavaNotifyError(1, "Err: Unable to set device property");
    SafeRelease(&pCamControl);
    return false;

}

JNIEXPORT jlong JNICALL Java_com_rockenbrew_JavaWMF_VideoDevice_GetDeviceProp
(JNIEnv* env, jobject thisObject, jlong cameraProp, jint type)
{

    // more settings; exposure, focus, etc...
    // https://docs.microsoft.com/en-us/windows/win32/api/strmif/ne-strmif-cameracontrolproperty

    ActiveDevice& actDevice = activeDevice[activeDeviceIndex];
    IMFMediaSource*& activeDeviceSource = actDevice.pVideoSource;

    // Query the capture filter for the IAMVideoProcAmp interface.
    IAMCameraControl* pCamControl = NULL;
    HRESULT hr = activeDeviceSource->QueryInterface(IID_IAMCameraControl, (void**)&pCamControl);
    if (FAILED(hr))
        goto doneErr;

    //CameraControl_Pan,
    //CameraControl_Tilt,
    //CameraControl_Roll,
    //CameraControl_Zoom,
    //CameraControl_Exposure,
    //CameraControl_Iris,
    //CameraControl_Focus

    long min, max, step, defaultVal, autoFlag, val; // Auto=1L/Manual=2L

    switch (type) {

    case PROP_VALUE:
    case PROP_AUTO_FLAG:

        hr = pCamControl->Get(cameraProp, &val, &autoFlag);
        if (SUCCEEDED(hr))
        {
            SafeRelease(&pCamControl);

            switch (type) {
            case PROP_VALUE: return static_cast<jlong>(val);
            case PROP_AUTO_FLAG: return static_cast<jlong>(autoFlag);
            }
        }
        else
            goto doneErr;

    default:

        hr = pCamControl->GetRange(cameraProp, &min, &max, &step, &defaultVal, &autoFlag);
        if (SUCCEEDED(hr))
        {

            SafeRelease(&pCamControl);
                
            switch (type) {
            case PROP_MIN: return static_cast<jlong>(min);
            case PROP_MAX: return static_cast<jlong>(max);
            case PROP_STEP: return static_cast<jlong>(step);
            case PROP_DEFAULT: return static_cast<jlong>(defaultVal);
            }
        }
        else
            goto doneErr;

        break;

    }

doneErr:

    //JavaNotifyError(1, "Err: Unable to get device property");
    SafeRelease(&pCamControl);
    return ERROR_VALUE_JLONG;

}


boolean StopStream()
{

    if (activeDevice[activeDeviceIndex].streaming == STREAMING)
        activeDevice[activeDeviceIndex].streaming = STOPPING; // request stop

    return true;
}


void Stream(UINT32 frameWidth, UINT32 frameHeight, UINT32 inputStreamLen, UINT32 outputStreamLen, int inputFormat, int returnFormat) {

    const char* streamName = "VStream1";

    JNIEnv* env;
    JavaVMAttachArgs args;
    args.version = jvm.version;
    args.name = (char*)streamName;
    args.group = NULL;
    jvm.vm->AttachCurrentThread((void**)&env, &args);

    ActiveDevice& actDevice = activeDevice[activeDeviceIndex];

    jobject& rcvr = actDevice.rcvr;

    // Find the id of the Java method to be called
    jclass rcvrClass = env->GetObjectClass(rcvr);
    jmethodID rcvrNotifyImageData = env->GetMethodID(rcvrClass, "notifyImageData", "([BJ)V");
    jmethodID rcvrNotifyEvent = env->GetMethodID(rcvrClass, "notifyEvent", "(I)V");
    jmethodID rcvrNotifyError = env->GetMethodID(rcvrClass, "notifyError", "(ILjava/lang/String;)V");

    IMFSourceReader* pReader = NULL;
    IMFAttributes* videoConfig = NULL;

    DWORD streamIndex, flags;
    IMFSample* pSample = NULL;
    LONGLONG timeStamp;
    long sampleCount = 0;

    // Create the source reader
    MFCreateAttributes(&videoConfig, 1);
    videoConfig->SetGUID(
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID); // look at video configs only only

    HRESULT hr = MFCreateSourceReaderFromMediaSource(actDevice.pVideoSource, videoConfig, &pReader);
    if (FAILED(hr))
    {

        string errMsg = "Err: MFCreateSourceReaderFromMediaSource";
        jstring jErrMsg = env->NewStringUTF(errMsg.c_str());
        env->CallObjectMethod(rcvr, rcvrNotifyError, // notify error
            ERROR_STREAM,
            jErrMsg); 
        goto done;
    }

    hr = pReader->SetCurrentMediaType(0, NULL, actDevice.pVideoMediaType);
    if (FAILED(hr))
    {
        string errMsg = "Err: SetCurrentMediaType";
        jstring jErrMsg = env->NewStringUTF(errMsg.c_str());
        env->CallObjectMethod(rcvr, rcvrNotifyError, // notify error
            ERROR_STREAM, jErrMsg);
        goto done;
    }

    env->CallObjectMethod(rcvr, rcvrNotifyEvent, STREAM_ON + actDevice.device.id); // notify stream on

    while (actDevice.streaming == STREAMING)// && failedAttempts < maxAttempts)
    {

        LONGLONG sampleDuration = NULL;

        HRESULT hr = pReader->ReadSample(
            MF_SOURCE_READER_FIRST_VIDEO_STREAM,		// Stream index (dwStreamIndex)
            0,							                // Flags (ie. MF_SOURCE_READER_CONTROLF_DRAIN)
            &streamIndex,								// Receives the actual stream index
            &flags,							            // Receives status flags; TODO: handle error, format changes (ie. MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED)
            &timeStamp,
            &pSample									// Receives the sample or NULL
        );
        if (FAILED(hr))
        {
            StopStream();
            if (sampleCount == 0) // if the very first image fails, it's probably that the device is in use
            {
                string errMsg = "Err: ReadSample";
                jstring jErrMsg = env->NewStringUTF(errMsg.c_str());
                env->CallObjectMethod(rcvr, rcvrNotifyError, // notify error
                    ERROR_STREAM_DEVICE_FIRST, jErrMsg);
            } 
            else
            {
                string errMsg = "Err: ReadSample";
                jstring jErrMsg = env->NewStringUTF(errMsg.c_str());
                env->CallObjectMethod(rcvr, rcvrNotifyError, // notify error
                    ERROR_STREAM_DEVICE_MID, jErrMsg);
            }
            goto done;
        }
        else
            if (pSample)
            {

                IMFMediaBuffer* pBuffer = NULL;
                BYTE* pData = NULL;
                                
                // handle sample buffer data
                DWORD cbBitmapData = 0;
                hr = pSample->ConvertToContiguousBuffer(&pBuffer);

                hr = pBuffer->Lock(&pData, NULL, &cbBitmapData);
                if (FAILED(hr))
                {
                    StopStream();
                    string errMsg = "Err: Lock";
                    jstring jErrMsg = env->NewStringUTF(errMsg.c_str());
                    env->CallObjectMethod(rcvr, rcvrNotifyError, // notify error
                        ERROR_STREAM, jErrMsg);
                    goto done;
                }
                else
                {
                    //
                    // format (as necessary) and return image streams
                    //

                    assert(cbBitmapData == inputStreamLen);

                    jbyteArray outputStream = env->NewByteArray(outputStreamLen);

                    if (inputFormat == FORMAT_YUY2 && (returnFormat == FORMAT_ORIGINAL || returnFormat == FORMAT_RGB24)) // YUY2 source > RGB24 output
                    {

                        jbyte* transform = new jbyte[outputStreamLen];

                        if (!YUY2toRGB24(pData, inputStreamLen, transform)) // transform yuy2 (bytes: "y1, cb, y2, cr, ...") to rgb24 (bytes: r, g, b, ...)
                        {
                            StopStream();
                            string errMsg = "Err: YUY2toRGB24";
                            jstring jErrMsg = env->NewStringUTF(errMsg.c_str());
                            env->CallObjectMethod(rcvr, rcvrNotifyError, // notify error
                                ERROR_STREAM, jErrMsg);
                            goto done;
                        }

                        env->SetByteArrayRegion(outputStream, 0, outputStreamLen, transform);

                        delete[] transform;

                    }
                    else // No transform; straight copy
                    {
                        env->SetByteArrayRegion(outputStream, 0, outputStreamLen, (jbyte*)pData);
                    }

                    // return image byte stream to java
                    env->CallObjectMethod(rcvr, rcvrNotifyImageData, outputStream, (jlong)timeStamp);

                    env->DeleteLocalRef(outputStream);

                }

                hr = pBuffer->Unlock();
                if (FAILED(hr))
                {
                    StopStream();
                    string errMsg = "Err: Unlock";
                    jstring jErrMsg = env->NewStringUTF(errMsg.c_str());
                    env->CallObjectMethod(rcvr, rcvrNotifyError, // notify error
                        ERROR_STREAM, jErrMsg);
                    goto done;
                }
                //pBuffer->Release();
                SafeRelease(&pBuffer);
            
                pSample->GetSampleDuration(&sampleDuration);
            }

        SafeRelease(&pSample);
        sampleCount++;

        //delay until next frame ready
        std::this_thread::sleep_for(std::chrono::nanoseconds(sampleDuration/2*100));

    }

done:

    actDevice.streaming = STOPPED;
    env->CallObjectMethod(rcvr, rcvrNotifyEvent, STREAM_OFF + actDevice.device.id); // notify stream off
    SafeRelease(&pSample);
    SafeRelease(&videoConfig);
    SafeRelease(&pReader);

    jvm.vm->DetachCurrentThread();
}


JNIEXPORT boolean JNICALL Java_com_rockenbrew_JavaWMF_VideoDevice_StartStream
(JNIEnv* env, jobject thisObject, jint returnFormat, jobject rcvr)
{

    if (activeDeviceCount > 0)
    {

        ActiveDevice& device = activeDevice[activeDeviceIndex];

        // Get Video Format GUID
        int inputFormat{ FORMAT_NONE };
        PROPVARIANT stVar = GetAttrVarByGUID(MF_MT_SUBTYPE, device.captureFormat);
        GUID videoFormatGUID = *stVar.puuid;

        // Ugly switch equiv.
        if (wcscmp(GetGUIDNameConst(videoFormatGUID), L"MFVideoFormat_RGB24") == 0) inputFormat = FORMAT_RGB24;
        else if (wcscmp(GetGUIDNameConst(videoFormatGUID), L"MFVideoFormat_YUY2") == 0) inputFormat = FORMAT_YUY2;
        else if (wcscmp(GetGUIDNameConst(videoFormatGUID), L"MFVideoFormat_MPG") == 0) inputFormat = FORMAT_MPG;
        else if (wcscmp(GetGUIDNameConst(videoFormatGUID), L"MFVideoFormat_NV12") == 0) inputFormat = FORMAT_NV12;

        // Get Frame Rate of given CaptureFormat
        UINT32 frameWidth, frameHeight;
        LogUINT32AsUINT64(GetAttrVarByGUID(MF_MT_FRAME_SIZE, device.captureFormat), frameWidth, frameHeight);

        // Set length of in/ouput byte arrays
        UINT32 inputStreamLen = (frameWidth * frameHeight * PX_BYTES_RGB); // assume RGB24
        if (inputFormat == FORMAT_YUY2) inputStreamLen = (frameWidth * frameHeight * PX_BYTES_YUY2);

        UINT32 outputStreamLen = (frameWidth * frameHeight * PX_BYTES_RGB);
        if (returnFormat == FORMAT_YUY2) outputStreamLen = (frameWidth * frameHeight * PX_BYTES_YUY2);

        device.rcvr = rcvr;

        device.streaming = STREAMING;

        Stream(frameWidth, frameHeight, inputStreamLen, outputStreamLen, inputFormat, returnFormat);

        return TRUE;
    }
    else
        return FALSE;

}


JNIEXPORT jboolean JNICALL Java_com_rockenbrew_JavaWMF_VideoDevice_StopStream
(JNIEnv* env, jobject thisObject)
{

    return StopStream();
}


JNIEXPORT jboolean JNICALL Java_com_rockenbrew_JavaWMF_VideoDevice_IsStreaming
(JNIEnv* env, jobject thisObject)
{
    if (activeDeviceCount > 0)
    {
        if (activeDevice[activeDeviceIndex].streaming == STREAMING)
            return TRUE;
        else
            return FALSE;
    }
    else
        return FALSE;
}


//On Sample callback object

class CaptureEngineOnSampleCB : public IMFCaptureEngineOnSampleCallback
{

    long m_cRef;
    HWND m_hwnd;

public:
    CaptureEngineOnSampleCB(HWND hwnd) : m_cRef(1), m_hwnd(hwnd), m_fSleeping(false) {}

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IMFCaptureEngineOnSampleCallback
    STDMETHODIMP OnSample(_In_ IMFSample* pSample);

    bool m_fSleeping;
};


STDMETHODIMP CaptureEngineOnSampleCB::QueryInterface(REFIID riid, void** ppv)
{
    static const QITAB qit[] =
    {
        QITABENT(CaptureEngineOnSampleCB, IMFCaptureEngineOnEventCallback),
        { 0 }
    };

    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CaptureEngineOnSampleCB::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CaptureEngineOnSampleCB::Release()
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0)
    {
        delete this;
    }
    return cRef;
}


// Callback method to receive events from the capture engine.
STDMETHODIMP CaptureEngineOnSampleCB::OnSample(_In_ IMFSample* pSample)
{

    const char* streamName = "VStream4";

    JNIEnv* env;
    JavaVMAttachArgs args;
    args.version = jvm.version;
    args.name = (char*)streamName;
    args.group = NULL;
    jvm.vm->AttachCurrentThread((void**)&env, &args);

    ActiveDevice& actDevice = activeDevice[activeDeviceIndex];
    jobject& rcvr = actDevice.rcvr;

    // Find the id of the Java method to be called
    jclass rcvrClass = env->GetObjectClass(rcvr);
    jmethodID rcvrNotifyImageData = env->GetMethodID(rcvrClass, "notifyImageData", "([BJ)V");
    jmethodID rcvrNotifyEvent = env->GetMethodID(rcvrClass, "notifyEvent", "(I)V");

    env->CallObjectMethod(rcvr, rcvrNotifyEvent, 2); // notify take photo


    // Set Video Format
    int inputFormat{ FORMAT_YUY2 };
    int returnFormat{ FORMAT_RGB24 };

    // Get Frame Rate of given CaptureFormat

    PROPVARIANT pFrameSizeVar;
    PropVariantInit(&pFrameSizeVar);
    HRESULT hr = actDevice.pPhotoMediaType->GetItem(MF_MT_FRAME_SIZE, &pFrameSizeVar);

    UINT32 frameWidth = 640;
    UINT32 frameHeight = 480;
    LogUINT32AsUINT64(pFrameSizeVar, frameWidth, frameHeight);

    // Set length of in/ouput byte arrays
    UINT32 inputStreamLen = (frameWidth * frameHeight * PX_BYTES_RGB); // assume RGB24
    if (inputFormat == FORMAT_YUY2) inputStreamLen = (frameWidth * frameHeight * PX_BYTES_YUY2);

    UINT32 outputStreamLen = (frameWidth * frameHeight * PX_BYTES_RGB); // RGB24

    if (pSample)
    {

        LONGLONG timeStamp = 0;
        IMFMediaBuffer* pBuffer = NULL;
        BYTE* pData = NULL;

        // handle sample buffer data
        DWORD cbBitmapData = 0;
        HRESULT hr = pSample->ConvertToContiguousBuffer(&pBuffer);

        pSample->GetSampleTime(&timeStamp);

        hr = pBuffer->Lock(&pData, NULL, &cbBitmapData);
        if (FAILED(hr))
        {
            StopStream();
            JavaNotifyError(1, "Err: Lock");
            goto doneErr;
        }
        else
        {
            //
            // format (as necessary) and return image streams
            //

            assert(cbBitmapData == inputStreamLen);

            jbyteArray outputStream = env->NewByteArray(outputStreamLen);

            if (inputFormat == FORMAT_YUY2 && (returnFormat == FORMAT_ORIGINAL || returnFormat == FORMAT_RGB24)) // YUY2 source > RGB24 output
            {

                jbyte* transform = new jbyte[outputStreamLen];

                if (!YUY2toRGB24(pData, inputStreamLen, transform)) // transform yuy2 (bytes: "y1, cb, y2, cr, ...") to rgb24 (bytes: r, g, b, ...)
                {
                    StopStream();
                    JavaNotifyError(1, "Err: YUY2toRGB24");
                    goto doneErr;
                }

                env->SetByteArrayRegion(outputStream, 0, outputStreamLen, transform);

                delete[] transform;

            }
            else // No transform; straight copy
            {
                env->SetByteArrayRegion(outputStream, 0, outputStreamLen, (jbyte*)pData);
            }

            // return image byte stream to java
            env->CallObjectMethod(rcvr, rcvrNotifyImageData, outputStream, static_cast<jlong>(1));

            // release
            env->DeleteLocalRef(outputStream);

        }

        hr = pBuffer->Unlock();
        if (FAILED(hr))
        {
            JavaNotifyError(1, "Err: Unlock");
            goto doneErr;
        }

        pData = NULL;
        SafeRelease(&pBuffer);
    }

    
    //SafeRelease(&pSample); // pSample->Release(); // Either one causes a memory leak. Not sure why; the callback object itself is released elsewhere, if that helps
    wait = false;
    jvm.vm->DetachCurrentThread();

    return S_OK;

doneErr:

    SafeRelease(&pSample);
    wait = false;

    return E_FAIL;

}



// The event callback object.
class CaptureEngineCB : public IMFCaptureEngineOnEventCallback
{
    long m_cRef;
    HWND m_hwnd;

public:
    CaptureEngineCB(HWND hwnd) : m_cRef(1), m_hwnd(hwnd), m_fSleeping(false) {}

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IMFCaptureEngineOnEventCallback
    STDMETHODIMP OnEvent(_In_ IMFMediaEvent* pEvent);

    bool m_fSleeping;
};


STDMETHODIMP CaptureEngineCB::QueryInterface(REFIID riid, void** ppv)
{
    static const QITAB qit[] =
    {
        QITABENT(CaptureEngineCB, IMFCaptureEngineOnEventCallback),
        { 0 }
    };

    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CaptureEngineCB::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CaptureEngineCB::Release()
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0)
    {
        delete this;
    }
    return cRef;
}

// Callback method to receive events from the capture engine.
STDMETHODIMP CaptureEngineCB::OnEvent(_In_ IMFMediaEvent* pEvent)
{
    // Post a message to the application window, so the event is handled 
    // on the application's main thread. 

    if (m_fSleeping != NULL)
    {
        // We're about to fall asleep, that means we've just asked the CE to stop the preview
        // and record.  We need to handle it here since our message pump may be gone.
        GUID    guidType;
        HRESULT hrStatus;
        HRESULT hr = pEvent->GetStatus(&hrStatus);
        if (FAILED(hr))
        {
            hrStatus = hr;
        }

        hr = pEvent->GetExtendedType(&guidType);
        if (SUCCEEDED(hr))
        {

        }

        return S_OK;
    }
    else
    {
        pEvent->AddRef();  // The application will release the pointer when it handles the message.
        PostMessage(m_hwnd, WM_APP_CAPTURE_EVENT, (WPARAM)pEvent, 0L);
    }

    return S_OK;
}

int CountStillModes(boolean photoOnly) {

    ActiveDevice& actDevice = activeDevice[activeDeviceIndex];

    if (!mfCaptureEngineDLLAvail)
    {
        JavaNotifyError(1, "Err: Capture Engine DLL unavailable.");
        return false;
    }


    IMFCaptureEngine* pCaptureEngine = NULL;
    IMFAttributes* pAttributes = NULL;
    CaptureEngineCB* pCallback = NULL;
    CaptureEngineOnSampleCB* pOnSampleCallback = NULL;
    IMFCaptureSource* pSource = NULL;

    // loop for available formats
    DWORD streamCount = 0;
    DWORD i = 0;
    UINT32 captureFormatCount = 0;
    boolean lastDevice = false;
    DWORD sourceStreamFormat = 0; // Any stream 
    if (photoOnly)
    {
        sourceStreamFormat = 0xFFFFFFFB; //(DWORD)MF_CAPTURE_ENGINE_FIRST_SOURCE_PHOTO_STREAM;
    }
    //else
    //{
    //    DWORD sourceStreamFormat = 0xFFFFFFFC; //(DWORD)MF_CAPTURE_ENGINE_FIRST_SOURCE_VIDEO_STREAM
    //}


    //
    // initialise the Capture Engine with desired Video Source
    //
    HRESULT hr = MFCreateCaptureEngine(&pCaptureEngine);
    if (FAILED(hr))
    {
        JavaNotifyError(1, "Err: MFCreateCaptureEngine");
        goto doneErr;
    }

    pCallback = new (std::nothrow) CaptureEngineCB(hwndMain);
    if (pCallback == NULL)
    {
        JavaNotifyError(1, "Err: CaptureEngineCB (out of memory)");
        hr = E_OUTOFMEMORY;
        goto doneErr;
    }

    hr = MFCreateAttributes(&pAttributes, 1);
    pAttributes->SetUINT32(MF_CAPTURE_ENGINE_USE_VIDEO_DEVICE_ONLY, TRUE); // look at video configs only only
    if (FAILED(hr))
    {
        JavaNotifyError(1, "Err: MFCreateAttributes (VIDEO_DEVICE_ONLY)");
        goto doneErr;
    }

    pCaptureEngine->Initialize(pCallback, pAttributes, NULL, actDevice.pVideoSource);
    if (FAILED(hr))
    {
        JavaNotifyError(1, "Err: Initialize");
        goto doneErr;
    }

    //
    // Look for "photo" image capability
    //
    hr = pCaptureEngine->GetSource(&pSource);
    if (FAILED(hr))
    {
        JavaNotifyError(1, "Err: GetSource");
        goto doneErr;
    }

    pSource->GetDeviceStreamCount(&streamCount);
    if (FAILED(hr))
    {
        JavaNotifyError(1, "Err: GetDeviceStreamCount");
        goto doneErr;
    }
    else
        JavaNotifyError(1, "Stream Count: " + std::to_string(streamCount));

    // Iterate through available media types
    do
    {

        IMFMediaType* pMFType = NULL;

        hr = MFCreateMediaType(&pMFType);
        if (FAILED(hr))
        {
            JavaNotifyError(1, "Err: CreateMediaType (1)");
            goto doneErr3;
        }

        hr = pSource->GetAvailableDeviceMediaType(
            (DWORD)sourceStreamFormat,
            (DWORD)i, // media type index 
            &pMFType);
        if (FAILED(hr))
        {
            lastDevice = true; // loop escape
            if (hr != MF_E_NO_MORE_TYPES) JavaNotifyError(1, "Err: GetAvailableDeviceMediaType, (@itr" + std::to_string(i) + ")");
            goto doneErr3;
        }
        else
        {
            captureFormatCount++;
        }

        UINT32 count;
        hr = pMFType->GetCount(&count);
        if (FAILED(hr))
        {
            JavaNotifyError(1, "Err: Attr Count (MediaType1)");
            goto doneErr3;
        }

        JavaNotifyError(1, "CAPTURE_FORMAT_INDEX = " + std::to_string(i)); // newline

        for (int j = 0; j < count; j++) // for each attribute (of each Capture Format)
        {

            string name;
            string value;

            WCHAR* pGuidName = NULL;
            WCHAR* pGuidValName = NULL;

            string specialAttrStr;
            WCHAR* pGUIDname = NULL;

            GUID guid;
            PROPVARIANT var;
            PropVariantInit(&var);

            pMFType->GetItemByIndex(j, &guid, &var);
            HRESULT hr = GetGUIDName(guid, &pGuidName);
            if (FAILED(hr))
            {
                JavaNotifyError(1, "Err: GetGUIDName (MediaType1)");
                goto doneErr2;
            }

            name = ToNarrow(pGuidName);

            hr = SpecialCaseAttributeValue(guid, var, specialAttrStr); // Special cases, including frame, frame rate, etc. ("I x J"-type dimensions) see WMFmisc.h (CM); CM edited this and sub functions (see WMFmisc.h)

            if (hr == S_FALSE) // not "special case"
            {

                switch (var.vt) // vt=variable type (see https://docs.microsoft.com/en-us/windows/win32/api/propidlbase/ns-propidlbase-propvariant)
                {
                case VT_UI4:
                    value = MSGtoString(L"%d", var.ulVal);
                    break;

                case VT_UI8:
                    value = MSGtoString(L"%I64d", var.uhVal);
                    break;

                case VT_R8:
                    value = MSGtoString(L"%f", var.dblVal);
                    break;

                case VT_CLSID:
                    hr = GetGUIDName(*var.puuid, &pGuidValName);
                    if (SUCCEEDED(hr))
                    {
                        value = MSGtoString(pGuidValName);
                    }
                    break;

                case VT_LPWSTR:
                    value = MSGtoString(var.pwszVal);
                    break;

                case VT_VECTOR | VT_UI1:
                    value = "<<byte array>>";
                    break;

                case VT_UNKNOWN:
                    value = "IUnknown";
                    break;

                default:
                    value = MSGtoString(L"Unexpected attribute type (vt = %d)", var.vt);
                    break;
                }
            }
            else
            {
                value = specialAttrStr;
            }


            JavaNotifyError(1, name + " = " + value);

        doneErr2:
            CoTaskMemFree(pGuidName);
            CoTaskMemFree(pGuidValName);
            PropVariantClear(&var);

        }

        i++;
        JavaNotifyError(1, ""); // newline

    doneErr3:
        SafeRelease(&pMFType);

    } while (!lastDevice);

    JavaNotifyError(1, "Available capture formats: " + std::to_string(captureFormatCount));

    SafeRelease(&pSource);
    SafeRelease(&pOnSampleCallback);
    SafeRelease(&pCallback);
    SafeRelease(&pAttributes);
    SafeRelease(&pCaptureEngine);

    return TRUE;

doneErr:

    SafeRelease(&pSource);
    SafeRelease(&pOnSampleCallback);
    SafeRelease(&pCallback);
    SafeRelease(&pAttributes);
    SafeRelease(&pCaptureEngine);

    return FALSE;

}

boolean Snap(int mediaTypeIndex, boolean photoOnly, int preShoot) // media type; if photo only is available, use only that; take a few shots to allow any auto white balance/focus
{

    ActiveDevice& actDevice = activeDevice[activeDeviceIndex];

    if (!mfCaptureEngineDLLAvail)
    {
        JavaNotifyError(1, "Err: Capture Engine DLL unavailable.");
        return false;
    }

    IMFCaptureEngine* pCaptureEngine = NULL;
    IMFAttributes* pAttributes = NULL;
    CaptureEngineCB* pCallback = NULL;
    CaptureEngineOnSampleCB* pOnSampleCallback = NULL;
    IMFCaptureSource* pSource = NULL;
    IMFMediaType*& pMediaType = actDevice.pPhotoMediaType;
    IMFCaptureSink* pSink = NULL;
    IMFCapturePhotoSink* pPhoto = NULL;


    DWORD sourceStreamFormat = 0; // Any stream 
    if (photoOnly)
    {
        sourceStreamFormat = 0xFFFFFFFB; //(DWORD)MF_CAPTURE_ENGINE_FIRST_SOURCE_PHOTO_STREAM;
    }
    //else
    //{
    //    DWORD sourceStreamFormat = 0xFFFFFFFC; //(DWORD)MF_CAPTURE_ENGINE_FIRST_SOURCE_VIDEO_STREAM
    //}

    //
    // initialise the Capture Engine with desired Video Source
    //
    HRESULT hr = MFCreateCaptureEngine(&pCaptureEngine);
    if (FAILED(hr))
    {
        JavaNotifyError(1, "Err: MFCreateCaptureEngine");
        goto doneErr;
    }

    pCallback = new (std::nothrow) CaptureEngineCB(hwndMain);
    if (pCallback == NULL)
    {
        JavaNotifyError(1, "Err: CaptureEngineCB (out of memory)");
        hr = E_OUTOFMEMORY;
        goto doneErr;
    }

    hr = MFCreateAttributes(&pAttributes, 1);
    pAttributes->SetUINT32(MF_CAPTURE_ENGINE_USE_VIDEO_DEVICE_ONLY, TRUE); // look at video configs only only
    if (FAILED(hr))
    {
        JavaNotifyError(1, "Err: MFCreateAttributes (VIDEO_DEVICE_ONLY)");
        goto doneErr;
    }

    pCaptureEngine->Initialize(pCallback, pAttributes, NULL, actDevice.pVideoSource);
    if (FAILED(hr))
    {
        JavaNotifyError(1, "Err: Initialize");
        goto doneErr;
    }


    //
    // Create Photo Sink.
    //
    hr = pCaptureEngine->GetSink(MF_CAPTURE_ENGINE_SINK_TYPE_PHOTO, &pSink);
    if (FAILED(hr))
    {
        JavaNotifyError(1, "Err: GetSink");
        goto doneErr;
    }

    hr = pSink->QueryInterface(IID_PPV_ARGS(&pPhoto));
    if (FAILED(hr))
    {
        JavaNotifyError(1, "Err: QueryInterface (photo sink)");
        goto doneErr;
    }


    //
    // set media type by index
    //

    hr = pCaptureEngine->GetSource(&pSource);
    if (FAILED(hr))
    {
        JavaNotifyError(1, "Err: GetSource");
        goto doneErr;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(250)); // delay, apparently required; for hardware to fire up, I guess

    //pSource->GetDeviceStreamCount(&streamCount); // optional
    //if (FAILED(hr))
    //{
    //    JavaNotifyError(1, "Err: GetDeviceStreamCount");
    //    goto doneErr;
    //}
    //else
    //    JavaNotifyError(1, "Stream Count: " + std::to_string(streamCount));

    hr = MFCreateMediaType(&pMediaType);
    if (FAILED(hr))
    {
        JavaNotifyError(1, "Err: CreateMediaType (2)");
        goto doneErr;
    }

    hr = pSource->GetAvailableDeviceMediaType(sourceStreamFormat, (DWORD)mediaTypeIndex, &pMediaType);
    if (FAILED(hr))
    {
        JavaNotifyError(1, "Err: GetAvailableDeviceMediaType (i" + std::to_string(mediaTypeIndex) + ")");
        goto doneErr;
    }

    hr = pSource->SetCurrentDeviceMediaType(sourceStreamFormat, pMediaType);
    if (FAILED(hr))
    {
        JavaNotifyError(1, "Err: SetCurrentDeviceMediaType");
        goto doneErr;
    }

    //
    // Set up engine streams
    //
    hr = pPhoto->RemoveAllStreams();
    if (FAILED(hr))
    {
        JavaNotifyError(1, "Err: RemoveAllStreams");
        goto doneErr;
    }

    DWORD dwSinkStreamIndex;
    hr = pPhoto->AddStream(
        sourceStreamFormat,
        pMediaType,
        NULL,
        &dwSinkStreamIndex);
    if (FAILED(hr))
    {
        JavaNotifyError(1, "Err: AddStream");
        goto doneErr;
    }


    //
    // Setup callback to receive image data
    //
    pOnSampleCallback = new (std::nothrow) CaptureEngineOnSampleCB(hwndMain);
    if (pOnSampleCallback == NULL)
    {
        JavaNotifyError(1, "Err: OnSampleCB");
        goto doneErr;
    }

    hr = pPhoto->SetSampleCallback(pOnSampleCallback);
    if (FAILED(hr))
    {
        JavaNotifyError(1, "Err: Set OnSampleCB");
        goto doneErr;
    }


    //
    // Request image data
    //

    //hr = pCaptureEngine->StartPreview(); // must configure preview sink

    //for (int i = 0; i < preShoot; i++) {
    //    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    //    hr = pCaptureEngine->TakePhoto();
    //}

    hr = pCaptureEngine->TakePhoto();
    if (FAILED(hr))
    {
        wait = false;
        JavaNotifyError(1, "Err: TakePhoto");
        goto doneErr;
    }
    else
    {
        wait = true;
    }

    while (wait)
    {
    }

    //hr = pCaptureEngine->StopPreview();

    pPhoto->Release();
    hr = pCaptureEngine->Release();
    

    SafeRelease(&pPhoto);
    SafeRelease(&pSink);
    SafeRelease(&pSource);
    SafeRelease(&pOnSampleCallback);
    SafeRelease(&pCallback);
    SafeRelease(&pAttributes);
    SafeRelease(&pCaptureEngine);

    return TRUE;

doneErr:

    SafeRelease(&pPhoto);
    SafeRelease(&pSink);
    SafeRelease(&pSource);
    SafeRelease(&pOnSampleCallback);
    SafeRelease(&pCallback);
    SafeRelease(&pAttributes);
    SafeRelease(&pCaptureEngine);

    return FALSE;
}


JNIEXPORT jboolean JNICALL Java_com_rockenbrew_JavaWMF_VideoDevice_GetImage
(JNIEnv* env, jobject thisObject, jobject rcvr, jint mediaTypeindex)
{

    ActiveDevice& actDevice = activeDevice[activeDeviceIndex];
    actDevice.rcvr = rcvr;

    // stick this on a thread so the callback can return before this function (and the rcvr object) go out of scope
    std::thread snapper(Snap, mediaTypeindex, false, 3);
    snapper.join();

    return true;
}

int GetActiveDeviceCount()
{
    return activeDeviceCount;
}

// TODO
//
// set max frame rate: https://docs.microsoft.com/en-us/windows/win32/medfound/how-to-set-the-video-capture-frame-rate