package com.rockenbrew.SJMI;

import java.awt.Color;
import java.awt.Graphics2D;
import java.awt.image.BufferedImage;
import java.util.Arrays;

public abstract class StreamProcessor {
	
	public static final int COLOUR_MODE_AVG = 0;
	public static final int COLOUR_MODE_R = 1;
	public static final int COLOUR_MODE_G = 2;
	public static final int COLOUR_MODE_B = 3;
	public static final int COLOUR_MODE_H = 4;
	public static final int COLOUR_MODE_S = 5;
	public static final int COLOUR_MODE_V = 6;
	public static final int COLOUR_MODE_L = 7;
	public static final int COLOUR_MODE_I = 8;
	public static final int COLOUR_MODE_J = 9;
	
	public abstract BufferedImage processImage(byte[] imageByteArray, int w, int h);
	
	public static final BufferedImage clone(BufferedImage image, int type) {
		
	    BufferedImage clone = new BufferedImage(image.getWidth(),
	            image.getHeight(), type);
	    Graphics2D g2d = clone.createGraphics();
	    g2d.drawImage(image, 0, 0, null);
	    g2d.dispose();
	    
	    return clone;
	}
	
	public static int[] GetCoordOfIndex(int i, int width)	{
		int x = i%width;		
		int y = (int)((i - x) / (double)width);
//		if (i--%width>0) y++;
		
		return new int[] {x, y};		
	}

	
	public static int GetIndexOfCoord(int x, int y, int width)	{
		return y * width + x;		
	}
	

	final static double maxIntensity = Hyp(1,1);
	
	public static int GetRGB(int colourIndex, int colourMode)	{
		
		switch (colourMode) {
	
		case COLOUR_MODE_R: return (int)((colourIndex >> 16) & 0x000000FF); // r
		case COLOUR_MODE_G: return (int)((colourIndex >> 8) & 0x000000FF);// g
		case COLOUR_MODE_B: return (int)((colourIndex) & 0x000000FF); // b
		
		default:

			int [] rgb = new int [3];
			rgb[0] = (colourIndex >> 16) & 0x000000FF;
			rgb[1] = (colourIndex >> 8) & 0x000000FF;
			rgb[2] = (colourIndex) & 0x000000FF;
			
			switch (colourMode)	{
			
			case COLOUR_MODE_AVG: 
				return (int)Math.round((rgb[0]+rgb[1]+rgb[2])/3F); // avg / intensity 0-255

			case COLOUR_MODE_H: // hue
				float[] hsb_h = Color.RGBtoHSB(rgb[0], rgb[1], rgb[2], null);
				return (int)Math.round(hsb_h[0]*360F);
			case COLOUR_MODE_S: // saturation
				float[] hsb_s = Color.RGBtoHSB(rgb[0], rgb[1], rgb[2], null);
				return (int)Math.round(hsb_s[1]*255F);
			case COLOUR_MODE_V:  // brightness
				float[] hsb_v = Color.RGBtoHSB(rgb[0], rgb[1], rgb[2], null);
				return (int)Math.round(hsb_v[2]*255F);

			case COLOUR_MODE_L: // luma 0-255; used by grid finder
				double[] rgbD = { (double)rgb[0], (double)rgb[1], (double)rgb[2] };		
				double[] y = { 0.21D, 0.72D, 0.07D }; // 709
				//			double[] y = { 0.22D, 0.72D, 0.6D };
				//			double[] y = { 0.30D, 0.59D, 0.11D }; // 601
				return (int)Math.round(WeightedAvg(y, rgbD));

			case COLOUR_MODE_I: // intensity2 0-255
				Arrays.sort(rgb);
				return (int)Math.round((rgb[0]+rgb[1])/2F);
				//			return (int)Math.round((rgb[0]+rgb[2])/2F);

			case COLOUR_MODE_J: // intensity1
				float[] hsb_i = Color.RGBtoHSB(rgb[0], rgb[1], rgb[2], null);
				double intensity = Hyp((1-hsb_i[1]), (hsb_i[2]));
				return (int)((intensity/maxIntensity)*255F); 

			default: return -1;

			}	
		}
	}
	
	
	static double Hyp(double a, double b)	{
		
		double c = Math.sqrt(Math.pow(a, 2)+Math.pow(b, 2));
		
		return c;
	}
	
	
	static double WeightedAvg(double[] weightFactor, double[] value)	{
		
		double weightedAvg=0;
		double sumWgtFactor=0;
		
		for (int n=0; n<weightFactor.length; n++)
			sumWgtFactor += weightFactor[n];
		
		for (int n=0; n<weightFactor.length; n++)
			weightedAvg += (weightFactor[n]/sumWgtFactor) * value[n];
				
		return weightedAvg;
	}

	
//	static ColorModel createColorModel(byte[] n, int size) {
//	 
//	byte[] r = new byte[size];
//	byte[] g = new byte[size];
//	byte[] b = new byte[size];
//	 
//	for (int i = 0; i < r.length; i+=2) {
//		r[i] = (byte) n[i*2];
//		g[i] = (byte) n[i*2];
//		b[i] = (byte) n[i*2];
//		
//		r[i+1] = (byte) n[(i+1)*2];
//		g[i+1] = (byte) n[(i+1)*2];
//		b[i+1] = (byte) n[(i+1)*2];
//	}
//	
//	int x=(int)Math.pow(2, 8);
//	
//	return new IndexColorModel(8, x, r,g,b, DataBuffer.TYPE_BYTE);
//
////	return new IndexColorModel(8, 2, new byte[]{
////			   (byte) 0xf, 0x0
////			 }, new byte[]{
////			   (byte) 0xf, 0x0
////			 }, new byte[]{
////			   (byte) 0xf, 0x0
////			 });
//	
//}

	
}
