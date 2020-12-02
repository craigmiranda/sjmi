package com.rockenbrew.SJMI;

import java.awt.image.BufferedImage;

public class ImageSink implements Sink, VideoDeviceSinkInterface {

		private volatile BufferedImage image;
		protected SJMI handler;
		
		public ImageSink(SJMI handler) {
			
			this.handler = handler;
		}
		
		@Override
		public void updateImage(BufferedImage newImage, long timeStamp)	{

			setImage(newImage);
		}
		
	
		@Override
		public void onLostDevice(int deviceIndex) {
			
		}
		
		public void onLostDevice(int deviceIndex, VideoDevice device) {
			
			handler.initialise();
			onLostDevice(deviceIndex);
		}


		@Override
		public void onNewDevice() {
			
			handler.initialise();
		}

		@Override
		public void onStreamOn() {
			
			
		}

		@Override
		public void onStreamOff() {
			
			
		}

		@Override
		public void onPhotoTaken() {
			
			
		}

		@Override
		public void onError(int errorCode, String errorMessage) {

			if (handler.getMonitorSink() != null)
				handler.getMonitorSink().onError(errorCode, errorMessage); // send to centralised error receiver
		}

		@Override
		public void onActivate(VideoDevice device) {
			
		}

		@Override
		public void onDeactivate() {
			
		}

		public BufferedImage getImage() {
			return image;
		}

		public void setImage(BufferedImage image) {
			this.image = image;
		}

		
}
