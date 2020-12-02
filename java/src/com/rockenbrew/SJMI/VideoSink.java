package com.rockenbrew.SJMI;

import java.awt.Graphics;
import java.awt.image.BufferedImage;

import javax.swing.JPanel;


public class VideoSink extends ImageSink {

	private JPanel viewFinder;
	
	public VideoSink(SJMI handler) {
		super(handler);
		super.handler = handler;
		
		viewFinder = new JPanel() {

			private static final long serialVersionUID = 4500622511448988659L;

			@Override
			public void paintComponent(Graphics g) {
				super.paintComponent(g);

				g.drawImage(getImage(), 0, 0, this);
			}

		};

	}
	
	@Override
	public synchronized void updateImage(BufferedImage newImage, long timeStamp)	{

		setImage(newImage);
		viewFinder.repaint();
	}
	
	public JPanel getViewFinderPane()	{
		return viewFinder;
	}
	
	
}
