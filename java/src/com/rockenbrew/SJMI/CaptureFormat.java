package com.rockenbrew.SJMI;

import java.util.HashMap;

public class CaptureFormat {

	public static String CAPTURE_FORMAT_INDEX = "CAPTURE_FORMAT_INDEX"; // Not part of WMF
	public static String FRAME_SIZE = "MF_MT_FRAME_SIZE";
	public static String VIDEO_FORMAT = "MF_MT_SUBTYPE";
	public static String FRAME_RATE = "MF_MT_FRAME_RATE";
	public static String FRAME_RATE_MIN = "MF_MT_FRAME_RATE_RANGE_MIN";
	public static String FRAME_RATE_MAX = "MF_MT_FRAME_RATE_RANGE_MAX";
	
	
	private HashMap<String,String> attributes;
	private double frameWidth=-1, frameHeight=-1; // default values, in case attribute not present
	private double frameRateN=-1, frameRateD=-1; // numerator & denominator
	private double frameRateMinN=-1, frameRateMinD=-1; // numerator & denominator
	private double frameRateMaxN=-1, frameRateMaxD=-1; // numerator & denominator
	
	public CaptureFormat(HashMap<String,String> attributes) {
		
		this.attributes = attributes;
		
		
		// for key attributes with AxB format string values, split and store double vals or whatever
		if (attributes.get(FRAME_SIZE) != null) {
			frameWidth = getAxBValue(this.attributes, FRAME_SIZE, true);
			frameHeight = getAxBValue(this.attributes, FRAME_SIZE, false);
		}
		
		if (attributes.get(FRAME_RATE) != null) {
			frameRateN = getAxBValue(this.attributes, FRAME_RATE, true);	
			frameRateD = getAxBValue(this.attributes, FRAME_RATE, false);	
		}

		if (attributes.get(FRAME_RATE_MIN) != null) {
			frameRateMinN = getAxBValue(this.attributes, FRAME_RATE_MIN, true);	
			frameRateMinD = getAxBValue(this.attributes, FRAME_RATE_MIN, false);	
		}
		
		if (attributes.get(FRAME_RATE_MAX) != null) {
			frameRateMaxN = getAxBValue(this.attributes, FRAME_RATE_MAX, true);	
			frameRateMaxD = getAxBValue(this.attributes, FRAME_RATE_MAX, false);	
		}
		
	}
	
	
	public String getCapFormatAttributeValueStr(String attribute)	{
		
		return attributes.get(attribute);
	}
	
	
	private static double getAxBValue(HashMap<String,String> attributes, String attr, boolean a)	{ // return pixel width as double from String (in format "%Wx%H")
		
		Object attrStr = attributes.get(attr);
				
		if (attrStr == null)
			return -1d;
		
		int xCharIndex = ((String)attrStr).indexOf('x');
		if (xCharIndex == -1)
			return -1d;
		
		String valueStr;
		if (a)
			valueStr = ((String)attrStr).substring(0, xCharIndex); // substring before the 'x'
		else
			valueStr = ((String)attrStr).substring(xCharIndex+1, ((String)attrStr).length()); // substring after the 'x'
		
		double dValue = -1; 
		
		try {
			dValue = (double)Double.valueOf(valueStr);
		} catch (NumberFormatException e) {
			return -1d;			
		}
		
		return dValue;
	}


	protected String getAttributeVal(String attrName) {
		
		Object o = attributes.get(attrName);
		
		if (o != null)	
			return (String)o;
		else
			return "null";
	}
	
	protected HashMap<String, String> getAttributes() {
		return attributes;
	}

	public double getFrameWidth() {
		return frameWidth;
	}

	public double getFrameHeight() {
		return frameHeight;
	}

	public double getFrameRateNumerator() {
		return frameRateN;
	}

	public double getFrameRateDenominator() {
		return frameRateD;
	}
	
	public double getFrameRate()	{
		
		return getFrameRateNumerator() / getFrameRateDenominator();
	}

	public double getFrameRateMinNumerator() {
		return frameRateMinN;
	}

	public double getFrameRateMinDenominator() {
		return frameRateMinD;
	}
	
	public double getMinFrameRate()	{
		
		return getFrameRateMinNumerator() / getFrameRateMinDenominator();
	}

	public double getFrameRateMaxNumerator() {
		return frameRateMaxN;
	}

	public double getFrameRateMaxDenominator() {
		return frameRateMaxD;
	}

	public double getMaxFrameRate()	{
		
		return getFrameRateMaxNumerator() / getFrameRateMaxDenominator();
	}

}
