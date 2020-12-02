package com.rockenbrew.SJMI;

import java.awt.Color;
import java.awt.image.BufferedImage;

public class StreamProcessorRGB24 extends StreamProcessor {

	
	public synchronized BufferedImage processImage(byte[] imageByteArray, int w, int h)	{

		BufferedImage image = new BufferedImage(w, h, BufferedImage.TYPE_INT_RGB);
		
		//
		// rgb24 input stream
		//

		for (int i=0; i<w*h; i++) {

			int[] xy = GetCoordOfIndex(i, w);
			int x = xy[0];
			int y = xy[1];

			int pxByteIndex = i*3;

			int r = (int)(imageByteArray[pxByteIndex] & 0xFF);
			int g = (int)(imageByteArray[pxByteIndex+1] & 0xFF);
			int b = (int)(imageByteArray[pxByteIndex+2] & 0xFF);

			try {
				image.setRGB(x, y, new Color(b,g,r).getRGB());
				
			} catch(IllegalArgumentException e) {
				image.setRGB(x, y, Color.RED.getRGB());
			}

		}
			
		//		DataBuffer dataBuffer = new DataBufferByte(byteArray, byteArray.length, 0);
		//		int scanlineStride = w*2; // YUY2
		//		SampleModel sampleModel = 
		////				new SinglePixelPackedSampleModel(
		////				DataBuffer.TYPE_BYTE, w, h, scanlineStride, new int[] {(byte)0xf});
		//				new MultiPixelPackedSampleModel(
		//						DataBuffer.TYPE_BYTE, w, h, 8, scanlineStride, 0);
		////				new PixelInterleavedSampleModel(
		////						DataBuffer.TYPE_BYTE, w, h, 2, scanlineStride, new int[] {(byte)0xf});
		//		WritableRaster raster = Raster.createWritableRaster(
		//						sampleModel, dataBuffer, new Point(0, 0));
		//		
		//		BufferedImage image = new BufferedImage(createColorModel(byteArray, w*h), raster, false, null);
		
		//image = processImage(image);
		
		BufferedImage finalImage = clone(image, image.getType());
				
		image.flush();
		
		return finalImage;
	}
	
	public synchronized BufferedImage processImage(BufferedImage image)	{
		
		return image;
	}
	
	
}
