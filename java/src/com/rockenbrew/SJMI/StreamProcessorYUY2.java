package com.rockenbrew.SJMI;

import java.awt.Color;
import java.awt.image.BufferedImage;

public class StreamProcessorYUY2 extends StreamProcessor {


	private static final int PX_BYTES_YUY2 = 2; // each pixel is represented by 2 bytes, sort of...
	private static final int PX_PAIR_BYTES_YUY2 = PX_BYTES_YUY2 * 2; // we'll look at 2 pixels (4 bytes) at a time
	
	@Override
	public synchronized BufferedImage processImage(byte[] imageByteArray, int w, int h) {

		BufferedImage image = new BufferedImage(w, h, BufferedImage.TYPE_INT_RGB);

		//
		// yuy2 input stream
		//

	    for (int i = 0; i < imageByteArray.length - PX_PAIR_BYTES_YUY2; i += PX_PAIR_BYTES_YUY2) { // pick up 4 bytes at a time (ie. 2 pixels)

	    	// calc which (pair of) pixel coordindates we're talking about
	    	int[] xy = GetCoordOfIndex(i/PX_BYTES_YUY2, w);
			int pxA_X = xy[0];
			int pxA_Y = xy[1];
			
	    	int[] xy2 = GetCoordOfIndex(i/PX_BYTES_YUY2+1, w);
			int pxB_X = xy2[0];
			int pxB_Y = xy2[1];
			
	        // get YUY2 bytes for 2 pixels
	        short aY = (short)imageByteArray[i]; // use 8byte ints for decimal arithmetic
	        short Cr = (short)imageByteArray[i + 1];
	        short bY = (short)imageByteArray[i + 2];
	        short Cb = (short)imageByteArray[i + 3];

	        int pxA = YUY2toRGB24(aY, Cr, Cb);
	        int pxB = YUY2toRGB24(bY, Cr, Cb);

			try {
				
				image.setRGB(pxA_X, pxA_Y, new Color(pxA).getRGB());
				
			} catch(IllegalArgumentException e) {
				image.setRGB(pxA_X, pxA_Y, Color.RED.getRGB());
			}
			
			try {
				
				image.setRGB(pxB_X, pxB_Y, new Color(pxB).getRGB());
				
			} catch(IllegalArgumentException e) {
				image.setRGB(pxB_X, pxB_Y, Color.RED.getRGB());
			}

		}
			
		image = processImage(image);
		
		return image;
	}
	
	public synchronized BufferedImage processImage(BufferedImage image)	{
		// after effects
		
		return image;
	}
	
	private int YUY2toRGB24(int Y, int Cr, int Cb) { // YUV2 (aka. YCbCr, aka. 4:2:2, aka. YUV)
		
		int C = Y - 16;
		int D = Cr - 128;
		int E = Cb - 128;
		int r = (298*C+516*D+128)/256;
		int g = (298*C-100*D-208*E+128)/256;
		int b = (298*C+409*E+128)/256;
		
		
		try {
			return new Color(r,g,b).getRGB();
		
		} catch(IllegalArgumentException e) {
			
			// clip to valid range
			if (r<0) r=0;
			else if (r>255) r=255;
			if (b<0) b=0;
			else if (b>255) b=255;
			if (g<0) g=0;
			else if (g>255) g=255;
			
			return new Color(r,g,b).getRGB();
		}
		
	}

	
}

