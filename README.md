# sjmi
Skinny Java Media Interface - a Java interface for Video Devices

SJMI is a Java library which allows Java to access Video Devices ("web cams" and suchlike) attached to the system. It has been designed to provide basic functionality and ease of use. It weighs under 100kb, give or take (excluding the JVM and VC++ runtimes).

Functionality:

* Identify video devices & properties thereof
* Select video device & set video format (resolution, etc)
* Stream and capture images (to Java BufferedImage(s))
* Adjust brightness, white balance, focus, etc. (where available)
* Video Format support for RGB24 & YUY2
  * Overridable functions to implement new video formats (NV12, MPG)

Pending functionality:

* Mac & Linux sub-libraries
* Video formats NV12 & MPG
* "Still pin" image/photo capture (1, see below)
* USB3.0 devices

Possible functionality (3):

* Multiple device support
* Audio device support

Misc:

* SJMI sub-library written in C++ accessed via JNI
  - hence depends on the VC++ 2015-2019 Redistributables
* SJMI sub-library depends on shared sub-sub-libraries, part of the Microsoft Media Foundation ("MMF") (3)
* It includes half an implementation of MMF CaptureEngine for still/photo capture (3)

Requirements:

* 64-bit machine (4)
* Windows 10 (probably Windows 8, maybe Win7)
* VC++ 2015-2019 Redistributables (https://support.microsoft.com/en-us/help/2977003/the-latest-supported-visual-c-downloads) (5)

Testing:

* Minimal (in progress,as at Nov2020)
* Four seperate Windows 10 machines w/ integrated cameras
* One external USB2.0 Microscope camera

Notes:

(1) Currently image capture is taken from the conventionaly video stream. Some devices, apparently, have "still pin" functionality, but none of seem to.

(2) Windows, for some reason, has at least four methods for video capture. 
 - DirectShow
 - IMFSourceReader (mfreadwrite.h; https://docs.microsoft.com/en-us/windows/win32/api/_mf/)
 - IMFCaptureEngine (mfcaptureengine.h; overlaps with IMFSourceReader methods; https://docs.microsoft.com/en-us/windows/win32/api/mfcaptureengine/nn-mfcaptureengine-imfcaptureengine)
 - Windows.Media.Devices (https://docs.microsoft.com/en-us/uwp/api/Windows.Media.Devices?redirectedfrom=MSDN&view=winrt-19041)
  
  The SJMI sub-library uses the what I'd call the IMFSourceReader method

(3) I personally, currently have no use for these features, so don't count on them appearing soon by my hand. Though, I don't believe they would be a huge effort.

(4) I simply haven't bothered to try compiling a 32-bit version. 

(5) SJMI offers error catching for this issue and allows the developer to provide an error message.
