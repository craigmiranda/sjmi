package com.rockenbrew.SJMI;

public class Receiver {

	VideoDevice device;
	Sink sink;

	public Receiver(VideoDevice device) {
		
		this.device = device;
	}

	public Receiver(Sink sink) {
		
		this.sink = sink;
	}
	
	
	public synchronized void notifyError(int errorCode, String message)	{
		// Pass error onto user overridable function
		
		if (message == null) {
			return;
		}

//		System.err.println(message + " (ee" + errorCode + ")");
		
		if (device != null)
		
			device.onError(errorCode, message);
		
		else if (sink != null)
			
			sink.onError(errorCode, message);
	}
	
	public synchronized void notifyImageData(byte[] rawImageData, long timeStamp)	{

		device.updateSink(rawImageData, timeStamp);	
	}

	
	public synchronized void notifyEvent(int deviceCode)	{

		int codeB = deviceCode % 1000;
		int codeA = deviceCode - codeB;

		if (device != null) { // if this is a device specific sink
			
			if (SJMI.debugMode) device.onError(0, "device: " + device.getFriendlyName() + ", event#: " + deviceCode);

			switch (codeA) {

			case EventCodes.STREAM_OFF:
				
				break;

			case EventCodes.STREAM_ON:
				
				break;

			case EventCodes.PHOTO_TAKEN:
				
				break;

			case EventCodes.DEVICE_LOST:

				device.onLostDevice();

				break;

			case EventCodes.DEVICE_NEW:

				device.onNewDevice();

				break;

			case EventCodes.DEVICE_ACTIVATED:

				break;

			case EventCodes.DEVICE_DEACTIVATED:

				break;
			}


		} else if (sink != null) { // if this is not a device specific sink
			
			if (SJMI.debugMode) sink.onError(0, "device: #" + codeB + ", event#: " + deviceCode);

			switch (codeA) {

			case EventCodes.DEVICE_LOST:
				
				sink.onLostDevice(codeB);

				break;

			case EventCodes.DEVICE_NEW:

				sink.onNewDevice();
				
				break;

			}	

		}
	}
	
	
}
