# **Web Interface Change Guide**

## **Streamer/OSD page:**
<img width="675" height="641" alt="Stremer-osd" src="https://github.com/user-attachments/assets/c9122532-3ace-4465-9236-4722ab90e533" />

### **Tabs**
<img width="345" height="38" alt="streamer-tabs" src="https://github.com/user-attachments/assets/dec5c340-f5e5-4b64-8dce-b41f73ece7a5" />

#### **Common Tab**

##### **\<Camera Select\>:**

<img width="199" height="37" alt="cam-select" src="https://github.com/user-attachments/assets/55d388ff-e528-4ac7-92b2-bd8b2df1275f" />

Select the active Preview Pan for Zoom function.  
**\[Camera 1\]**:  Pan and Tilt Camera  
**\[Camera 2\]**:  Fix Camera  
**\[\</\>\]**:  Full view on selected preview.

##### 
##### **\<Restart and Configuration Save\>:**

<img width="350" height="42" alt="config" src="https://github.com/user-attachments/assets/2601343f-4576-4821-9b3c-2708563d049e" />


##### **\<Motor Pan and Tilt Control\>:**  

<img width="315" height="149" alt="ptz-control" src="https://github.com/user-attachments/assets/c1e5bb1f-2d07-4a17-886e-5593733b031a" />
	
  Move the Pan and Tilt camera.  (double click at center to calibrate and home)

##### **\<Camera View Control\>:**
<img width="395" height="91" alt="cam-control" src="https://github.com/user-attachments/assets/06e641ae-6d8e-4d16-b04b-91e3c842cff0" />

**\[Dual Camera\]**:    
Enable or disable the second preview pan.  When disabled, the preview pan will change with the Camera Select.  

**\[Zoom Enable\]**:  (per camera)  
		When disabled, the view will reset back to the full view and reset the zoom settings.  
**\[Flip horizontally\]** and **\[Flip vertically\]**:  (per camera)  
		Flip the view in the preview pan(s) based on the Image Control  \<Camera Setting Select\>.

##### **\<Light Control\>:**


<img width="392" height="135" alt="light-control" src="https://github.com/user-attachments/assets/0d28ddb1-d885-421b-91d0-b4ee17629e9f" />

**\[White Light\]**:  White LED at Pan and Tilt camera   
**\[IR-850 Ligh\]**:  IR LED for both cameras  
**\[IR-940 Light\]**:  IR LED for both cameras  
**\[IR Cut\]**:  IR filter for both cameras

##### **\<Image Tuning Settings\>:  (per camera)**


  \[Camera Setting Select\]:  Select Camera 1, 2 or both.  


<img width="514" height="471" alt="Screenshot 2026-07-09 191930" src="https://github.com/user-attachments/assets/94dc30eb-636d-4dfd-92f1-b67b71018f5b" />

##### **\<Dual Preview Pans\>:  (click to select, and select by the \<Camera Select\>)**

<img width="625" height="183" alt="dual-view" src="https://github.com/user-attachments/assets/13a007dd-7acb-4a03-b19d-75a34f8a5749" />

Selecting zoom area by click and drag.  (up to 2.5x, double click to reset back to full view \- 1x)  
<img width="578" height="170" alt="zoom-view" src="https://github.com/user-attachments/assets/7af5a306-c22a-4294-9f40-64b8c38cb2bb" />

#### **Stream 0 and Stream 1 Tabs:**  --	Combined stream settings with OSD settings.

#### **Audio Tab:** 	-- No change.

## **Motion Map Page:**

<img width="428" height="500" alt="motion-clip map" src="https://github.com/user-attachments/assets/ee863383-ba53-4df3-b41b-c0fb83406f07" />


##### **\<Motion Guard Control\>:**

<img width="393" height="167" alt="motion-menu" src="https://github.com/user-attachments/assets/f44bc684-ebc5-4bd6-95d7-18c9e967cdc4" />

**\[Enable motion guard\]**:  Enable Motion Detection  
**\[Monitor Stream\]**:  ROI Map Pan stream selection (only the stream 0 has the motor functions).  
**\[Map Mode\]**:   on \- Enable Map Mode / off \- Box Mode   
**\[Auto Tracking\]**:  Enable Auto Tracking on detected a motion (stream 0 only)
**\[Auto Home\]**:  Enable back to home location after motion detection (stream 0 only)
**\[Sensibility\]**:  Motion detection sensibility  
**\[Delay between events\]**:  action after multiple detection on the event (debounce)  
**\[Light off delay (sec)\]**:  white light off delay (if enabled)  
**\[Home delay (sec)**\]:  Auto Home delay (if enabled, stream 0 only)


##### **\<ROI Map Pan\>:**

<img width="408" height="247" alt="box-mode" src="https://github.com/user-attachments/assets/17fe837b-c233-4902-913b-db4f5a5495f7" />

**Map Mod off**:    
Select a single ROI (Region Of Interest) area by click and drag.

<img width="405" height="230" alt="map mode" src="https://github.com/user-attachments/assets/b3431695-40c7-45cb-9c51-8470a8cc6c66" />

**Map Mode on**:  

- 8 x 6 grid boxs, 48 ROIs.
- Select ROI regions by click and drag.  The grid box turn LIGHT RED when selected. 
- Single click on a grid box to select or deselect the grid box  
- (with a ctrl key down)  Single click on a grid box to center it. (pan and tilt)  
- Double click select all grid boxes.

  ROI will take effect after [Save ROIs].
  
##### **\<ROI Functions\>:**

<img width="243" height="36" alt="ROI-menu" src="https://github.com/user-attachments/assets/347d18ec-e3ef-420e-ad9b-c72830859b51" />

**\[Clear All ROIs\]**:  Clear ROI on both MAP mode and Box mode.  
**\[Save ROIs\]**:  Save ROI setting to the Configuration file (in /etc/prudynt.json.  When a setting changed, the button turn RED, and back to GRAY after saved)
