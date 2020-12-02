# Getting Started

You might first like to see if it works - I'm curious - in which case, run the example.

CamTest 

The example application returns all the available devices and resolutions. It also shows a live feed, with selectable input.

* Download ZIP and unzip directory
* run compiled executable '<SJMI_DIRECTORY>\camtest_example\distributable\exe\CamTest.exe'
* OR From command prompt, run:

  'java -jar -Djava.library.path=<SJMI_DIRECTORY>\camtest_example\distributable\jar\ <SJMI_DIRECTORY>\camtest_example\distributable\jar\CamTest.jar'

(the Djava.library.path must include 'sjmi_win_lib.dll')

* OR run Windows installer '<SJMI_DIRECTORY>\camtest_example\distributable\setup\RbCamTestSetup.exe'
 (the installer includes the VC++15-19 runtime installation)
 
 
All being well, you can have a poke around in the code. The library has two components, the Java bit and the C++ bit. (JNI joins them up.)

Java bit

(Assumes Eclipse is IDE...)

* Download ZIP and unzip directory
* (Eclipse) File > Open Projects From File System...
* You should have two packages
  * CamTest: this is a simple implementation of the library consisting of just CamTest.java
  * com.rockenbrew.SJMI: this is the java half of the library
* Configure CamTest build path (Right click 'CamTest' package in the Package Explorer, Build Path > Configure Build Path)
  * Libraries tab: Ensure smji.jar appears. If not, Add Jar... > from '<SJMI_DIRECTORY>/java/dist/sjmi.jar'
  * Source tab: Native library location select '<SJMI_DIRECTORY>/java/lib'
* Run CamTest
* Configure com.rockenbrew.SJMI build path (as above)
 * Source tab: Native library location select '<SJMI_DIRECTORY>/java/lib'
* To create the Jar, either  
  * File > Export, Java > Jar File, ...
  * Create a build manfiest for the Jar build (find mine, for example, sjmi_build_manfiest_win.xml)

 C++ bit
 
TODO ...
