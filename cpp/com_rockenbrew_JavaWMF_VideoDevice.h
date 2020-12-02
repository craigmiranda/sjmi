/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_rockenbrew_JavaWMF_VideoDevice */

#ifndef _Included_com_rockenbrew_JavaWMF_VideoDevice
#define _Included_com_rockenbrew_JavaWMF_VideoDevice
#ifdef __cplusplus
extern "C" {
#endif
#undef com_rockenbrew_JavaWMF_VideoDevice_DEVICE_TYPE_CAMERA
#define com_rockenbrew_JavaWMF_VideoDevice_DEVICE_TYPE_CAMERA 0L
#undef com_rockenbrew_JavaWMF_VideoDevice_FORMAT_DEFAULT
#define com_rockenbrew_JavaWMF_VideoDevice_FORMAT_DEFAULT 1L
#undef com_rockenbrew_JavaWMF_VideoDevice_FORMAT_RGB24
#define com_rockenbrew_JavaWMF_VideoDevice_FORMAT_RGB24 1L
#undef com_rockenbrew_JavaWMF_VideoDevice_FORMAT_YUY2
#define com_rockenbrew_JavaWMF_VideoDevice_FORMAT_YUY2 2L
#undef com_rockenbrew_JavaWMF_VideoDevice_FORMAT_MPG
#define com_rockenbrew_JavaWMF_VideoDevice_FORMAT_MPG 3L
#undef com_rockenbrew_JavaWMF_VideoDevice_FORMAT_NV12
#define com_rockenbrew_JavaWMF_VideoDevice_FORMAT_NV12 4L
#undef com_rockenbrew_JavaWMF_VideoDevice_PROP_BRIGHTNESS
#define com_rockenbrew_JavaWMF_VideoDevice_PROP_BRIGHTNESS 0L
#undef com_rockenbrew_JavaWMF_VideoDevice_PROP_CONTRAST
#define com_rockenbrew_JavaWMF_VideoDevice_PROP_CONTRAST 1L
#undef com_rockenbrew_JavaWMF_VideoDevice_PROP_HUE
#define com_rockenbrew_JavaWMF_VideoDevice_PROP_HUE 2L
#undef com_rockenbrew_JavaWMF_VideoDevice_PROP_SATURATION
#define com_rockenbrew_JavaWMF_VideoDevice_PROP_SATURATION 3L
#undef com_rockenbrew_JavaWMF_VideoDevice_PROP_SHARPNESS
#define com_rockenbrew_JavaWMF_VideoDevice_PROP_SHARPNESS 4L
#undef com_rockenbrew_JavaWMF_VideoDevice_PROP_GAMMA
#define com_rockenbrew_JavaWMF_VideoDevice_PROP_GAMMA 5L
#undef com_rockenbrew_JavaWMF_VideoDevice_PROP_COLORENABLE
#define com_rockenbrew_JavaWMF_VideoDevice_PROP_COLORENABLE 6L
#undef com_rockenbrew_JavaWMF_VideoDevice_PROP_WHITEBALANCE
#define com_rockenbrew_JavaWMF_VideoDevice_PROP_WHITEBALANCE 7L
#undef com_rockenbrew_JavaWMF_VideoDevice_PROP_BACKLIGHTCOMPENSATION
#define com_rockenbrew_JavaWMF_VideoDevice_PROP_BACKLIGHTCOMPENSATION 8L
#undef com_rockenbrew_JavaWMF_VideoDevice_PROP_GAIN
#define com_rockenbrew_JavaWMF_VideoDevice_PROP_GAIN 9L
#undef com_rockenbrew_JavaWMF_VideoDevice_PROP_PAN
#define com_rockenbrew_JavaWMF_VideoDevice_PROP_PAN 0L
#undef com_rockenbrew_JavaWMF_VideoDevice_PROP_TILT
#define com_rockenbrew_JavaWMF_VideoDevice_PROP_TILT 1L
#undef com_rockenbrew_JavaWMF_VideoDevice_PROP_ROLL
#define com_rockenbrew_JavaWMF_VideoDevice_PROP_ROLL 2L
#undef com_rockenbrew_JavaWMF_VideoDevice_PROP_ZOOM
#define com_rockenbrew_JavaWMF_VideoDevice_PROP_ZOOM 3L
#undef com_rockenbrew_JavaWMF_VideoDevice_PROP_EXPOSURE
#define com_rockenbrew_JavaWMF_VideoDevice_PROP_EXPOSURE 4L
#undef com_rockenbrew_JavaWMF_VideoDevice_PROP_IRIS
#define com_rockenbrew_JavaWMF_VideoDevice_PROP_IRIS 5L
#undef com_rockenbrew_JavaWMF_VideoDevice_PROP_FOCUS
#define com_rockenbrew_JavaWMF_VideoDevice_PROP_FOCUS 6L
#undef com_rockenbrew_JavaWMF_VideoDevice_PROP_VALUE
#define com_rockenbrew_JavaWMF_VideoDevice_PROP_VALUE 0L
#undef com_rockenbrew_JavaWMF_VideoDevice_PROP_VALUE_MIN
#define com_rockenbrew_JavaWMF_VideoDevice_PROP_VALUE_MIN 1L
#undef com_rockenbrew_JavaWMF_VideoDevice_PROP_VALUE_MAX
#define com_rockenbrew_JavaWMF_VideoDevice_PROP_VALUE_MAX 2L
#undef com_rockenbrew_JavaWMF_VideoDevice_PROP_VALUE_STEP
#define com_rockenbrew_JavaWMF_VideoDevice_PROP_VALUE_STEP 3L
#undef com_rockenbrew_JavaWMF_VideoDevice_PROP_VALUE_DEFAULT
#define com_rockenbrew_JavaWMF_VideoDevice_PROP_VALUE_DEFAULT 4L
#undef com_rockenbrew_JavaWMF_VideoDevice_PROP_VALUE_AUTO_FLAG
#define com_rockenbrew_JavaWMF_VideoDevice_PROP_VALUE_AUTO_FLAG 5L
/*
 * Class:     com_rockenbrew_JavaWMF_VideoDevice
 * Method:    GetImage
 * Signature: (Lcom/rockenbrew/JavaWMF/Receiver;I)Z
 */
JNIEXPORT jboolean JNICALL Java_com_rockenbrew_JavaWMF_VideoDevice_GetImage
  (JNIEnv *, jobject, jobject, jint);

/*
 * Class:     com_rockenbrew_JavaWMF_VideoDevice
 * Method:    StartStream
 * Signature: (ILcom/rockenbrew/JavaWMF/Receiver;)Z
 */
JNIEXPORT jboolean JNICALL Java_com_rockenbrew_JavaWMF_VideoDevice_StartStream
  (JNIEnv *, jobject, jint, jobject);

/*
 * Class:     com_rockenbrew_JavaWMF_VideoDevice
 * Method:    StopStream
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_rockenbrew_JavaWMF_VideoDevice_StopStream
  (JNIEnv *, jobject);

/*
 * Class:     com_rockenbrew_JavaWMF_VideoDevice
 * Method:    IsStreaming
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_rockenbrew_JavaWMF_VideoDevice_IsStreaming
  (JNIEnv *, jobject);

/*
 * Class:     com_rockenbrew_JavaWMF_VideoDevice
 * Method:    Activate
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL Java_com_rockenbrew_JavaWMF_VideoDevice_Activate
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_rockenbrew_JavaWMF_VideoDevice
 * Method:    Deactivate
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL Java_com_rockenbrew_JavaWMF_VideoDevice_Deactivate
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_rockenbrew_JavaWMF_VideoDevice
 * Method:    GetRawAvailCaptureFormat
 * Signature: ()[[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_com_rockenbrew_JavaWMF_VideoDevice_GetRawAvailCaptureFormat
  (JNIEnv *, jobject);

/*
 * Class:     com_rockenbrew_JavaWMF_VideoDevice
 * Method:    SetActiveCaptureFormat
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL Java_com_rockenbrew_JavaWMF_VideoDevice_SetActiveCaptureFormat
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_rockenbrew_JavaWMF_VideoDevice
 * Method:    SetVideoProp
 * Signature: (JJ)Z
 */
JNIEXPORT jboolean JNICALL Java_com_rockenbrew_JavaWMF_VideoDevice_SetVideoProp
  (JNIEnv *, jobject, jlong, jlong);

/*
 * Class:     com_rockenbrew_JavaWMF_VideoDevice
 * Method:    GetVideoProp
 * Signature: (JI)J
 */
JNIEXPORT jlong JNICALL Java_com_rockenbrew_JavaWMF_VideoDevice_GetVideoProp
  (JNIEnv *, jobject, jlong, jint);

/*
 * Class:     com_rockenbrew_JavaWMF_VideoDevice
 * Method:    SetDeviceProp
 * Signature: (JJ)Z
 */
JNIEXPORT jboolean JNICALL Java_com_rockenbrew_JavaWMF_VideoDevice_SetDeviceProp
  (JNIEnv *, jobject, jlong, jlong);

/*
 * Class:     com_rockenbrew_JavaWMF_VideoDevice
 * Method:    GetDeviceProp
 * Signature: (JI)J
 */
JNIEXPORT jlong JNICALL Java_com_rockenbrew_JavaWMF_VideoDevice_GetDeviceProp
  (JNIEnv *, jobject, jlong, jint);

#ifdef __cplusplus
}
#endif
#endif
