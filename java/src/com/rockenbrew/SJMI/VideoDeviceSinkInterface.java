package com.rockenbrew.SJMI;

import java.awt.image.BufferedImage;

public interface VideoDeviceSinkInterface extends Sink {

	public void updateImage(BufferedImage newImage, long timeStamp);
	public void onStreamOn();
	public void onStreamOff();
	public void onPhotoTaken();
	public void onActivate(VideoDevice device);
	public void onDeactivate();

}
