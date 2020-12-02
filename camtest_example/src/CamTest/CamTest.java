package CamTest;

import java.awt.BorderLayout;
import java.awt.Desktop;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.WindowEvent;
import java.awt.event.WindowListener;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.net.URISyntaxException;
import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;

import javax.swing.DefaultComboBoxModel;
import javax.swing.JComboBox;
import javax.swing.JEditorPane;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.event.HyperlinkEvent;
import javax.swing.event.HyperlinkListener;

import com.rockenbrew.JavaWMF.*;

public class CamTest {

	private JFrame frame = new JFrame();
	private JPanel mainPanel = new JPanel();
	private JComboBox<String> deviceDropDown = new JComboBox<String>();
	private JTextArea log = new JTextArea();
	
	private SJMI sjmi;
	private VideoSink videoSink;
	
	public static void main(String[] args) {

		
		 javax.swing.SwingUtilities.invokeLater(new Runnable() {
	          
	        	public void run() {
	        		
	        		if (!SJMI.isOs64bit())
	        			catchLibError("x86");
	        		
	        		if (!SJMI.isLibAvailable() || !SJMI.isLibAccessible())
	        			catchLibError(SJMI.getLibError());
//	        		else       		
	        			new CamTest(); // launch with or without the library
	            }
	        });
	        
		}

	public CamTest() {

		sjmi = new SJMI();
		
		boolean debugMode = true;
		SJMI.setDebugMode(debugMode);
		
		// draw screen

		// default size
		final int viewFinderWidth = 640;
		final int viewFinderHeight = 480;
		
		frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

		mainPanel.setLayout(new BorderLayout());

		videoSink = new VideoSink(sjmi);		
		videoSink.getViewFinderPane().setBackground(java.awt.Color.BLACK);
		videoSink.getViewFinderPane().setPreferredSize(new Dimension(viewFinderWidth, viewFinderHeight));
		videoSink.getViewFinderPane().setMaximumSize(new Dimension(viewFinderWidth, viewFinderHeight));
		mainPanel.add(videoSink.getViewFinderPane(), BorderLayout.CENTER);

		mainPanel.add(deviceDropDown, BorderLayout.NORTH);
		
		log = new JTextArea();
		log.setLineWrap(true);
		log.setEditable(false);
		
		JScrollPane scroll  = new JScrollPane(log, 
				JScrollPane.VERTICAL_SCROLLBAR_ALWAYS,
				JScrollPane.HORIZONTAL_SCROLLBAR_AS_NEEDED);
		scroll.setPreferredSize(new Dimension(1, 150));			
		scroll.setMaximumSize(new Dimension(1, 300));

		
		mainPanel.add(scroll, BorderLayout.SOUTH);
		
		frame.add(mainPanel);
		
		frame.pack();
		frame.setVisible(true);
		
		sjmi.setMonitorSink(new Sink()	{

			@Override
			public void onLostDevice(int deviceIndex) {
				
				logEntry("Lost device");
			}

			@Override
			public void onNewDevice() {

				logEntry("New device");
			}

			@Override
			public void onError(int errorCode, String errorMessage) {
				
				// handle errors
				switch (errorCode) {
				
				case ErrorCodes.ERROR_STREAM_DEVICE_FIRST :
				
					logEntry("Error: Device stream unavailable. Check device is not in use. (e" + errorCode + ")");
					break;
					
				case 0: // debugging message
					
					logEntry(errorMessage);
					break;
					
				default:
					
					logEntry(errorMessage + " (e" + errorCode + ")");
					break;
				}
				
			}
		});
		
		sjmi.initialise();
		
		logDeviceData();
		
		listDevices(Arrays.copyOf(sjmi.getDevices().toArray(), sjmi.getDevices().size(), VideoDevice[].class));
		streamFromDeviceByIndex(0);
		
//		final int defaultDeviceIndex=0;
//		final int defaultDeviceAttrSetIndex=6;
			
//		final int defaultDeviceIndex=1;
//		final int defaultDeviceAttrSetIndex=16;
			
//		final int defaultDeviceIndex=2;
//			final int defaultDeviceAttrSetIndex=2;
			

					
//		if (sjmi.initialise()) { // initialise Windows Media Foundation
//
//			if (debugMode) System.out.println("initialised (" + sjmi.getDevices().size() + ")");
//
//			listDevices(Arrays.copyOf(sjmi.getDevices().toArray(), sjmi.getDevices().size(), VideoDevice[].class));
//			
//			if (defaultDeviceIndex < sjmi.getDevices().size()) { // desired device index available
//
//				final VideoDevice device = sjmi.getDeviceByIndex(defaultDeviceIndex); // get one device
//
//				if (device.activate()) { // activate device
//
//					if (debugMode) System.out.println("cam active");
//						
////					device.debug_printAllDeviceAttrSets(); // print avail capture formats
//						
//					if (device.setActiveCaptureFormat(defaultDeviceAttrSetIndex)) { // set one available capture format as active
//
//						if (debugMode) System.out.println("cam format set");
//
//						device.debug_printDeviceActiveAttrSet();	// print selected capture format attribute set
//
////						device.getAvailResolutions();
////						device.debug_printAllVideoProps();
////						device.debug_printAllDeviceProps();
//
////						device.setPhotoSink(videoSink);
////						device.getImage(1000 * 10000); // 1s = 1,000,000,000ns, but Mediafoundation works in 100ns clips, so you want 'seconds * 10,000,000' or 'milliseconds * 10,000'
////						device.prerelease_getImage2(defaultDeviceAttrSetIndex);					
//
//						device.setVideoSink(videoSink);
//						device.startStream();
//
////						device.setProcessor(new StreamProcessorRGB24() {
////
////							@Override
////							public BufferedImage processImage(BufferedImage image)	{
////
////								int w = image.getWidth();
////								int h = image.getHeight();
////
////								//
////								// rgb24 input stream
////								//
////
////								for (int y=0; y<h; y++)
////									for (int x=0; x<w; x++) {
////
////										int colourIndex = image.getRGB(x, y);
////
////										//											int r = (int)(ImageManipulator.GetRGB(colourIndex, StreamProcessor.COLOUR_MODE_R));
////										//											int g = (int)(ImageManipulator.GetRGB(colourIndex, StreamProcessor.COLOUR_MODE_G));
////										//											int b = (int)(ImageManipulator.GetRGB(colourIndex, StreamProcessor.COLOUR_MODE_B));
////
////										//											int newColourIndex = colourIndex;
////
////										int newColourIndex = (colourIndex << 0);// & 0x000000FF;
////
////										//											int r2 = r;
////										//											int g2 = g;
////										//											int b2 = b;
////
////										//											int n = 1;
////
////										//											int r2 = (r << 4) & 0x000000FF;
////										//											int g2 = (g << 4) & 0x000000FF;
////										//											int b2 = (b << 4) & 0x000000FF;
////
////										try {
////
////											image.setRGB(x, y, new Color(newColourIndex).getRGB());
////											//												image.setRGB(x, y, new Color(r2,g2,b2).getRGB());
////
////										} catch(IllegalArgumentException e) {
////											image.setRGB(x, y, Color.MAGENTA.getRGB());
////										}
////
////									}
////
////								return image;
////							}
////
////						});
//							
//						try {
//							Thread.sleep(500); // wait for stream to start
//						} catch (InterruptedException e) {
//						}
//
//						if (device.isStreaming())	{ // check success
//
//							frame.addMouseListener(new MouseListener() {
//
//								@Override
//								public void mouseClicked(MouseEvent arg0) {
//
//
//								}
//
//								@Override
//								public void mouseEntered(MouseEvent arg0) {}
//
//								@Override
//								public void mouseExited(MouseEvent arg0) {}
//
//								@Override
//								public void mousePressed(MouseEvent arg0) {}
//
//								@Override
//								public void mouseReleased(MouseEvent arg0) {}
//							});
//
//						} else {
//
//							System.out.println("stream failed");
//						}
//
//					}
//					else
//						if (debugMode) System.out.println("set cam format failed");
//
//				}
//
//			}
//
//		
//		}

		
		frame.addWindowListener(new WindowListener() {

			public void windowActivated(WindowEvent arg0) {}

			public void windowClosed(WindowEvent arg0) {}

			@Override
			public void windowClosing(WindowEvent arg0) {

				// release device and WMF resources
				sjmi.uninitialise();
			}

			public void windowDeactivated(WindowEvent arg0) {}

			public void windowDeiconified(WindowEvent arg0) {}

			public void windowIconified(WindowEvent arg0) {}

			public void windowOpened(WindowEvent arg0) {}

		});
		
	}
	
	
	public void listDevices(VideoDevice[] devices) {
		
		DefaultComboBoxModel<String> deviceModel = new DefaultComboBoxModel<String>();
		
		for (int i=0; i<devices.length; i++)
			deviceModel.addElement(devices[i].getFriendlyName());
		
		deviceDropDown.addActionListener(new ActionListener() {

			@Override
			public void actionPerformed(ActionEvent ev) {
				
				streamFromDeviceByIndex(deviceDropDown.getSelectedIndex());
			}
						
		});
		
		deviceDropDown.setModel(deviceModel);
	}
	
	
	public void streamFromDeviceByIndex(int selectedDeviceIndex) {
	
		if (sjmi.isInitialised()) {
			
			// deactive any active devices
			int[] activeDeviceIndices = sjmi.getActiveDeviceIndex();
	
			for (int i=0; i<activeDeviceIndices.length; i++) {				
				sjmi.getDeviceByIndex(i).deactivate();
			}
			
			VideoDevice selectedDevice = sjmi.getDeviceByIndex(selectedDeviceIndex);
			
			selectedDevice.setVideoSink(videoSink);
			
			selectedDevice.activate();
//			selectedDevice.debug_printAllDeviceAttrSets();
			
			int cpCount = selectedDevice.getCaptureFormatCount();
			int cpIndex = 0;
			
			while (cpIndex < cpCount && !selectedDevice.setActiveCaptureFormat(cpIndex))
				cpIndex++;
			
			selectedDevice.startStream();
		}
		
	}
	
	
	public void logDeviceData()	{
		
		
		for (VideoDevice d : sjmi.getDevices()) {
			
			d.setPhotoSink(new ImageSink(sjmi));
			d.activate();
			
			logEntry(d.getFriendlyName());
			d.printAvailResolutions();

			d.deactivate();
			
			logEntry("");
		}
		
	}

	public void logEntry(String logData)	{
		
		log.append(logData + "\n");
		log.setRows(log.getLineCount());
		
	}
	
	public static void catchLibError(String error)	{

		// known errors
		final String errRedist2019 = "Can't find dependent libraries";
		final String err32bitJREMsg = "Can't load AMD 64-bit .dll on a IA 32-bit platform";
		final String errNoDLLMsg = "no sjmi_win_lib in java_library_path";
		
		final String errorLink = "http://www.rockenbrew.com/error/reporterror.php?error=";

		String title = "Video Device error";
		StringBuffer msg = new StringBuffer(error);
		String urlEncoded = "unknown";
		
		// remove part of error w/ user path to .dll
		int pathEndIndex = error.indexOf(": ");
		if (pathEndIndex>-1) error = error.substring(pathEndIndex+2); 
		
		try {
		
			urlEncoded = URLEncoder.encode(error, StandardCharsets.UTF_8.toString());
		
		} catch (UnsupportedEncodingException e) {
			
		}

		
		// for copying style
	    JLabel defaultStyle = new JLabel();
	    Font font = defaultStyle.getFont();
	    
	 // create some css from the label's font
	    StringBuffer style = new StringBuffer("font-family:" + font.getFamily() + ";");
	    style.append("font-weight:" + (font.isBold() ? "bold" : "normal") + ";");
	    style.append("font-size:" + font.getSize() + "pt;");
		
	    if (!SJMI.isOs64bit()) {
	    	
	    	title = "x86 / 32-bit System Unsupported";
	    	msg = new StringBuffer("<html><body style=\"" + style + "\">"
					+ "Video device access requires a 64-bit system."
					+ "<br/>Please refer to the website for details: <a href=\"" + errorLink + urlEncoded + "\">rockenbrew.com/error</a>"
					+ "</body></html>");
			
	    }
	    else 
	    	if (!SJMI.isLibAvailable())	{ // unsatisfied link exception error
			
			title = "Video Device Library Error";

			if (error.contains(errRedist2019)) {

				msg = new StringBuffer("<html><body style=\"" + style + "\">"
						+ "A minor Windows update is required for video device access."
						+ "<br/>Please refer to the website for details: <a href=\"" + errorLink + urlEncoded + "\">rockenbrew.com/error</a>"
						+ "</body></html>");
			} 
			else if (error.contains(err32bitJREMsg)) {

				msg = new StringBuffer("<html><body style=\"" + style + "\">"
						+ "A 64-bit Java (JRE) installation is required."
						+ "<br/>Please install from: <a href=\"www.https://www,java.com/\">java.com</a>"
						+ "<br/>Alternatively, refer to the website for further details: <a href=\"" + errorLink + urlEncoded + "\">rockenbrew.com/error</a>"
						+ "</body></html>");
			} 
			else if (error.contains(errNoDLLMsg)) {

				msg = new StringBuffer("<html><body style=\"" + style + "\">"
						+ "Missing 'sjmi_win_lib.dll'. Please try to re-install."
						+ "<br/>If this persists, please report via the website: <a href=\"" + errorLink + urlEncoded + "\">rockenbrew.com/error</a>"
						+ "</body></html>");
			}
			else {
				
				msg = new StringBuffer("<html><body style=\"" + style + "\">"
						+ "Unknown error ('" + error + "')"
						+ "<br/>Please report the error via the website for help: <a href=\"" + errorLink + urlEncoded + "\">rockenbrew.com/error</a>"
						+ "</body></html>");
				
			}
				
		} else if (!SJMI.isLibAccessible())	{ // security exception error

			title = "Video Device Security Error";
			msg = new StringBuffer("<html><body style=\"" + style + "\">"
					+ "Unable to access video device drivers. Please disable or exclude this app from any anti-virus software."
					+ "<br/>Alternatively, report the error via the website for help: <a href=\"" + errorLink + urlEncoded + "\">rockenbrew.com/error</a>"
					+ "</body></html>");
		}

		// html content
	    JEditorPane htmlContent = new JEditorPane("text/html", msg.toString());
	    
		 // launch links in browser
	    htmlContent.addHyperlinkListener(new HyperlinkListener()
	    {
	        @Override
	        public void hyperlinkUpdate(HyperlinkEvent e)
	        {
	            
	        	if (e.getEventType().equals(HyperlinkEvent.EventType.ACTIVATED))
					try {
						Desktop.getDesktop().browse(e.getURL().toURI());
					} catch (IOException | URISyntaxException e1) {}
	            
	        }

	    });
	    
	    htmlContent.setEditable(false);
	    htmlContent.setBackground(defaultStyle.getBackground());
	    
		JOptionPane.showMessageDialog(null, htmlContent, title, JOptionPane.ERROR_MESSAGE);
		
	}

}
	
	

