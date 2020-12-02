package com.rockenbrew.SJMI;

import java.awt.image.BufferedImage;
import java.util.HashMap;
import java.util.Map.Entry;

public class VideoDevice {

	public static final int DEVICE_TYPE_CAMERA = 0;
		
	// values must match Cpp library (see misc.h)
	public static final int FORMAT_DEFAULT = 1;
	public static final int FORMAT_RGB24 = 1;
	public static final int FORMAT_YUY2 = 2;
	public static final int FORMAT_MPG = 3;
	public static final int FORMAT_NV12 = 4;

	public static String FORMAT_DEFAULT_STR = "MFVideoFormat_RGB24";
	public static String FORMAT_RGB24_STR = "MFVideoFormat_RGB24";
	public static String FORMAT_YUY2_STR = "MFVideoFormat_YUY2";
	public static String FORMAT_MPG_STR = "MFVideoFormat_MPG";
	public static String FORMAT_NV12_STR = "MFVideoFormat_NV12";
	
	public static final int PROP_BRIGHTNESS = 0;
	public static final int PROP_CONTRAST = 1;
	public static final int PROP_HUE = 2;
	public static final int PROP_SATURATION = 3;
	public static final int PROP_SHARPNESS = 4;
	public static final int PROP_GAMMA = 5;
	public static final int PROP_COLORENABLE= 6;
	public static final int PROP_WHITEBALANCE = 7;
	public static final int PROP_BACKLIGHTCOMPENSATION = 8;
	public static final int PROP_GAIN = 9;
	
    public static final int PROP_PAN = 0;
    public static final int PROP_TILT = 1;
    public static final int PROP_ROLL = 2;
    public static final int PROP_ZOOM = 3;
    public static final int PROP_EXPOSURE = 4;
    public static final int PROP_IRIS = 5;
    public static final int PROP_FOCUS = 6;
	
	public static final int PROP_VALUE = 0;
	public static final int PROP_VALUE_MIN = 1;
	public static final int PROP_VALUE_MAX = 2;
	public static final int PROP_VALUE_STEP = 3;
	public static final int PROP_VALUE_DEFAULT = 4;
	public static final int PROP_VALUE_AUTO_FLAG = 5;
	

	public static long PROP_ERROR_VALUE = Long.MIN_VALUE+1;
	
	private SJMI handler;
	
	private String friendlyName;
	private String uniqueID;
	private int id;
	private int type;
	private boolean available; // no unplugged
	private boolean active;
	private boolean ready; // ie. capture format set
	private CaptureFormat[] captureFormats;
	private int activeCapFormatIndex;
	private int returnFormat; // return in RGB24 or original (ie. Cpp to do YUY2->RGB transform)
	private transient double frameWidth, frameHeight;
	private StreamProcessor processor;
//	private boolean streaming; // instead, check directly with the hardware
	
	private boolean streamMode;
	private long streamStartNanoT;
	private long waitForNanoT;
	
	private ImageSink videoSink;
	private ImageSink photoSink;
	
	private Thread streamer;
	
	public VideoDevice(SJMI handler, int id, String friendlyName, String uniqueID, boolean available)	{
		
		this.handler =  handler;
		this.setId(id);
		this.setFriendlyName(friendlyName);
		this.setAvailable(available);
		this.setActive(false);
		this.activeCapFormatIndex = -1;
		this.frameWidth = -1;
		this.frameHeight= -1; 
		this.setReturnFormat(VideoDevice.FORMAT_DEFAULT); // default
		this.setProcessor(new StreamProcessorRGB24()); // default processor is RGB24
		this.streamStartNanoT = -1;
		this.waitForNanoT = -1;
		this.streamMode = false;
		
	}
	
	
	public boolean activate()	{
		
		if (Activate(this.getId())) {
			if (prepare())	{
			
				setActive(true);
				if (SJMI.debugMode) onError(0, "device: #" + this.getId() + ", event#: " + EventCodes.DEVICE_ACTIVATED); // just to provide some symmetry with the 'deactivated' event that comes from the DLL
				return true;
			}
			
			onActivatedDevice();
		}
		else
			onError(ErrorCodes.ERROR_ACTIVATE, "Err: Unable to activate device");
		
		return false;
	}

	public boolean deactivate()	{
		
		return Deactivate(this.getId());
	}
	
	private boolean prepare()	{
		// identify available capture formats (frame size, frame rate, colour mode)
		
		String[][] rawCapFormatAttrs = GetRawAvailCaptureFormat();

		if (rawCapFormatAttrs.length > 0) {
			
			this.assembleCaptureFormat(rawCapFormatAttrs);			
		
			return true;
		}
		else
			return false;
	}
	
	public void assembleCaptureFormat(String[][] captureFormatsArray)	{
		// Attributes of device are returned as pairs; all names & values are pre-converted to java strings (in C++)
		// Attribute name (ie. GUID)
		// Attribute value
		// See (for example): https://docs.microsoft.com/en-us/windows/win32/medfound/mf-mt-frame-size-attribute

		captureFormats = new CaptureFormat[captureFormatsArray.length]; // (suppressed warning)
		
		for (int i=0; i<captureFormatsArray.length; i++) {
			
			HashMap<String, String> attrMap = new HashMap<String, String>();
			
			attrMap.put(CaptureFormat.CAPTURE_FORMAT_INDEX, String.valueOf(i)); // add an index as reference to send back to C++
			
			for (int j=0; j<captureFormatsArray[i].length; j+=2) // number of Attrs varies with different media types
				attrMap.put(captureFormatsArray[i][j], captureFormatsArray[i][j+1]);
			
			captureFormats[i] = new CaptureFormat(attrMap);
		}
	}
	
	public void GetCaptureFormatByIndex() {
		
		
	}
	
	public boolean setActiveCaptureFormat(int captureFormatIndex) {
		
		String sourceVideoFormat = getDeviceAttrVal(captureFormatIndex, CaptureFormat.VIDEO_FORMAT);
		
		// if the source format is not one (of the two) with inbuilt support AND the user has not overridden the return format throw error
		// if the user has overridden, then let them try their own StreamProcessor
		if ((!sourceVideoFormat.matches(FORMAT_RGB24_STR) && !sourceVideoFormat.matches(FORMAT_YUY2_STR))
				&& getReturnFormat() == VideoDevice.FORMAT_DEFAULT) {
		
			onError(1, "Format (" + sourceVideoFormat + ") is not supported. No StreamProcessor available");
			return false;
		}
		
		if (SetActiveCaptureFormat(captureFormatIndex)) {
		
			this.activeCapFormatIndex = captureFormatIndex;
			
			setReady(true);
			return true;
			
		} else
			return false;
		
	}

	public boolean setReady(boolean ready) {
		
		if (activeCapFormatIndex >= 0 &&
				(videoSink != null || photoSink != null))
			this.ready = true;
		
		return this.ready;
	}
	
	public String getDeviceAttrVal(int captureFormatIndex, String attrName) {
		
		return captureFormats[captureFormatIndex].getAttributeVal(attrName);
	}

	public String getActiveDeviceAttrVal(String attrName) {
		
		return captureFormats[activeCapFormatIndex].getAttributeVal(attrName);
	}
	
	public void debug_printAllDeviceAttrSets()	{
		
		for (int i=0; i<captureFormats.length; i++)
			debug_printDeviceAttrSet(i);
	}
	
	public void debug_printDeviceActiveAttrSet()	{
		
		if (!isActive() || activeCapFormatIndex == -1) {
			
			if (SJMI.debugMode) onError(1, "Err: Device inactive or no capture format set (active:" + isActive() + ", CFindex:" + activeCapFormatIndex + ")");
			return;
		}
			
		debug_printDeviceAttrSet(activeCapFormatIndex);
	}
	
	
	public void debug_printDeviceAttrSet(int i)	{
		// See debug_printAllCaptureFormat
		// See: https://docs.microsoft.com/en-us/windows/win32/medfound/mf-mt-frame-size-attribute

        for (Entry<String, String> dP : captureFormats[i].getAttributes().entrySet()) {

        	onError(0, dP.getKey() + " = " + dP.getValue());
        }
        
	}

	
	public void printAvailResolutions()	{

		for (CaptureFormat cf : captureFormats)
			onError(0, cf.getAttributeVal(CaptureFormat.FRAME_SIZE) +
					" (" +
					cf.getAttributeVal(CaptureFormat.VIDEO_FORMAT) +
					")");
	}
	
	
	public int getCaptureFormatCount()	{

		return captureFormats.length;
	}
	
	
	protected synchronized void updateSink(byte[] imageData, long timeStamp)	{
		
		// update sink(s) with new image
		// process raw image byte data to produce image
		
		if (ready) { // capture format and sinks are set
	
			final int w = (int)getFrameWidth();
			final int h = (int)getFrameHeight();
	
			BufferedImage newImage = processor.processImage(imageData, w, h);
			
			//
			// video stream
			//
			
			if (videoSink != null) 
				videoSink.updateImage(newImage, timeStamp);
			
			//
			// photo
			//
						
			if (photoSink != null) {
				
					if (waitForNanoT >= 0 // if asked to wait
							&& streamStartNanoT == -1) // and not already set
						streamStartNanoT = timeStamp; //  set rebased time for delay calc
			
					if (timeStamp >= (streamStartNanoT+waitForNanoT)) // wait time has elapsed
					{
	
						photoSink.updateImage(newImage, timeStamp);
	
						if (!streamMode) 
							stopStream(); // if we were already streaming, don't stop
	
						onPhotoTaken(); // trigger event for user to handle
					}
			}
			
		}
	}
	
	
	public boolean startStream()	{
		
		if (available & ready) {
		
			streamer = new Streamer(this, false, 0);
			streamer.start();
		
			streamMode = true;
			
			return true;
		}
		else
		{
			onError(1, "Err: Device not avilable or no capture format set");
			return false;
		}
	}

	public boolean stopStream()	{
		 
		streamStartNanoT = -1;
		waitForNanoT = -1; // reset
		streamMode = false;
		
		return StopStream();
	}
	
	
	public boolean getImage(long delay)	{
		
		waitForNanoT = delay;
		
		if (available && ready && !IsStreaming()) {
			
			streamer = new Streamer(this, false, 0);
			streamer.start();
		
			return true;
		}
		else
		{
			onError(1, "Err: device not present or no capture format set");
			return false;
		}
	}
	
	
	public boolean prerelease_getImage2(int deviceAttrSetIndex)	{
	
		if (available && ready && !IsStreaming()) {
			
			streamer = new Streamer(this, true, deviceAttrSetIndex); // use photo mode, where availiable (MFCaptureEngine)
			streamer.start();
		
			return true;
		}
		else
		{
			onError(1, "Err: device not present or no capture format set");
			return false;
		}
	}
	
	class Streamer extends Thread {

		VideoDevice device;
		boolean still;
		int stillDevAttrSetIndex;
		
		public Streamer(VideoDevice device, boolean still, int stillDevAttrSetIndex)	{
			
			setName("SJMI-Image Stream");
			this.device = device;		
			this.still = still;
			this.stillDevAttrSetIndex = stillDevAttrSetIndex;
		}
		
		@Override
    	public void run() {
    	
			if (still) {

				GetImage(new Receiver(videoSink), stillDevAttrSetIndex);
				
			} else
				StartStream(device.getReturnFormat(), new Receiver(device));
				
    	} 
    }

	
	public void debug_printAllVideoProps()	{
		
		final String[] prop = {"Brightness", "Contrast", "Hue", "Saturation", "Sharpness", "Gamma", "Color Enable", "White Balance", "Backlight Compenstaion", "Gain" };
		final String[] autoFlagStrOpt = {"Absolute", "Auto", "Manual", "3", "4", "5", "6", "7", "8", "9", "Relative" };
		
		for (long i = 0; i < prop.length; i++) {
		
			String autoFlagStr = "Error";
			long autoFlag = getVideoProp(i, VideoDevice.PROP_VALUE_AUTO_FLAG);
			if (autoFlag != PROP_ERROR_VALUE)
				autoFlagStr = autoFlagStrOpt[(int)autoFlag];
			
			long value = getVideoProp(i, VideoDevice.PROP_VALUE);
			
			if (value != PROP_ERROR_VALUE)
				onError(0, prop[(int) i] + ": " + value + "\n"
						+ " Range (min - max): " + getVideoProp(i, VideoDevice.PROP_VALUE_MIN) + " - " + getVideoProp(i, VideoDevice.PROP_VALUE_MAX) + "\n"
						+ " Step: " + getVideoProp(i, VideoDevice.PROP_VALUE_STEP) + "\n"
						+ " Default: " + getVideoProp(i, VideoDevice.PROP_VALUE_DEFAULT) + "\n"
						+ " Auto: " + autoFlagStr		
						);
			else
				onError(0, prop[(int) i] + " not available\n");
		}
	}


	public void debug_printAllDeviceProps()	{
		
		final String[] prop = {"Pan", "Tilt", "Roll", "Zoom", "Exposure", "Iris", "Focus" };
		final String[] autoFlagStrOpt = {"Absolute", "Auto", "Manual", "3", "4", "5", "6", "7", "8", "9", "Relative" };
		
		for (long i = 0; i < prop.length; i++) {
		
			String autoFlagStr = "Error";
			long autoFlag = getVideoProp(i, VideoDevice.PROP_VALUE_AUTO_FLAG);
			if (autoFlag != PROP_ERROR_VALUE)
				autoFlagStr = autoFlagStrOpt[(int)autoFlag];
			
			long value = getDeviceProp(i, VideoDevice.PROP_VALUE);
			
			if (value != PROP_ERROR_VALUE)
				onError(0, prop[(int) i] + ": " + value + "\n"
						+ " Range (min - max): " + getDeviceProp(i, VideoDevice.PROP_VALUE_MIN) + " - " + getDeviceProp(i, VideoDevice.PROP_VALUE_MAX) + "\n"
						+ " Step: " + getDeviceProp(i, VideoDevice.PROP_VALUE_STEP) + "\n"
						+ " Default: " + getDeviceProp(i, VideoDevice.PROP_VALUE_DEFAULT) + "\n"
						+ " Auto: " + autoFlagStr		
						);
			else
				onError(0, prop[(int) i] + " not available\n");
		}
	}
	
	protected synchronized void onLostDevice()	{

		// default behaviour
		if (isStreaming()) {
			setAvailable(false);
			setActive(false);
		}
		
		if (videoSink != null) videoSink.onLostDevice(this.id, this);
		else if (photoSink != null) photoSink.onLostDevice(this.id, this);
	}
	
	protected synchronized void onNewDevice()	{
		if (videoSink != null) videoSink.onNewDevice();
		else if (photoSink != null) photoSink.onNewDevice();	
	}
	
	protected synchronized void onStreamOn() {
		if (videoSink != null) videoSink.onStreamOn();
		else if (photoSink != null) photoSink.onStreamOn();
	}

	protected synchronized void onStreamOff() {
		if (videoSink != null) videoSink.onStreamOff();
		else if (photoSink != null) photoSink.onStreamOff();
	}

	protected synchronized void onPhotoTaken() {
		if (videoSink != null) videoSink.onPhotoTaken();
		else if (photoSink != null) photoSink.onPhotoTaken();
	}

	protected synchronized void onActivatedDevice() {

	}
	
	protected synchronized void onDeactivatedDevice() {
		if (videoSink != null) videoSink.onDeactivate();
		else if (photoSink != null) photoSink.onDeactivate();
	}
	
	protected synchronized void onError(int errorCode, String errorMessage) {
		if (videoSink != null) videoSink.onError(errorCode, errorMessage);
		else if (photoSink != null) photoSink.onError(errorCode, errorMessage);
	}
	
	public boolean setVideoProp(long videoPropIndex, long value) {
		return SetVideoProp(videoPropIndex, value);
	}

	public long getVideoProp(long videoPropIndex, int type) {
		return GetVideoProp(videoPropIndex, type);
	}
	public boolean setDeviceProp(long devicePropIndex, long value) {
		return SetDeviceProp(devicePropIndex, value);
	}
	
	public long getDeviceProp(long devicePropIndex, int type) {
		return GetDeviceProp(devicePropIndex, type);
	}	
	
	public boolean isStreaming()	{
		
		return IsStreaming();
	}
	
//	public void setStreaming(boolean streaming) {
//
//		this.streaming = streaming;
//	}
	
	public double getFrameWidth()	{

		if (!active || activeCapFormatIndex == -1)
			return -1;

		return captureFormats[activeCapFormatIndex].getFrameWidth();
	}
	
	
	public double getFrameHeight()	{
		
		if (!active || activeCapFormatIndex == -1)
			return -1;

		return captureFormats[activeCapFormatIndex].getFrameHeight();
	}
	
	public SJMI getHandler() {
		return handler;
	}
	
	public Sink getVideoSink() {
		return videoSink;
	}

	public void setVideoSink(ImageSink sink) {
		this.videoSink = sink;
		setReady(true);
	}

	public Sink getPhotoSink() {
		return photoSink;
	}

	public void setPhotoSink(ImageSink sink) {
		this.photoSink = sink;
		setReady(true);
	}
	
	public String getFriendlyName() {
		return friendlyName;
	}
	
	public void setFriendlyName(String friendlyName) {
		this.friendlyName = friendlyName;
	}

	public String getUniqueID() {
		return uniqueID;
	}


	public void setUniqueID(String uniqueID) {
		this.uniqueID = uniqueID;
	}

	public int getType() {
		return type;
	}

	public void setType(int type) {
		this.type = type;
	}

	public boolean isActive() {
		return active;
	}

	public void setActive(boolean active) {
		this.active = active;
	}

	public boolean isAvailable() {
		return available;
	}

	public void setAvailable(boolean available) {
		this.available = available;
	}
	
	public int getId() {
		return id;
	}

	public void setId(int id) {
		this.id = id;
	} 

	public int getReturnFormat() {
		return returnFormat;
	}

	public void setReturnFormat(int returnFormat) {
		this.returnFormat = returnFormat;
	}

	public StreamProcessor getProcessor() {
		return processor;
	}

	public void setProcessor(StreamProcessor processor) {
		this.processor = processor;
	}
	
	private native boolean GetImage(Receiver receiver, int stillDevAttrSetIndex);
	private native boolean StartStream(int returnFormat, Receiver receiver);
	private native boolean StopStream();
	private native boolean IsStreaming();
	private native boolean Activate(int deviceIndex);	
	private native boolean Deactivate(int deviceIndex);	
	private native String[][] GetRawAvailCaptureFormat();
	private native boolean SetActiveCaptureFormat(int captureFormatIndex);
	private native boolean SetVideoProp(long videoPropIndex, long value);
	private native long GetVideoProp(long videoPropIndex, int type);
	private native boolean SetDeviceProp(long devicePropIndex, long value);
	private native long GetDeviceProp(long devicePropIndex, int type);
}
