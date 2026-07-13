7/13/2026

This project is based on the wonderful work by team of thingino-firmware and Prudynt-T. Some features been added to the Prudynt-T and WebUI for my own experiment.

The code was based on the “master+eebcb12” build with Oct. 2025 JASON WebUI. And, it has only been tested on <szt_ct211_t23n_gc1084_dual_atbm6012bx> dual sensors camera, which is not a supported platform by the team currently.  Only related and changed source code is posted.

Changed/Added Features:

- Dual sensor support for the T23 based camera. The sensor is switched at T23 MIPI interface with a MIPI switch and feed to two seperate stream path through the encoders. This is different with the front end switching I did in the earlier project, and is how the Ingenic based T23 design is done in the T23 comercial deployment. 
- Add dual-sensor/dual-stream RTSP/JPEG streaming and dual-view to WebUI.  
- Integrate GPIO control into Prudynt for better access for the lights and the sensor switch.
- Add Tiled/Map multi-ROIs to motion control. This allows detection of regions of interest only to avoid false alarm.  The monitored stream is selectable between stream 0 and 1.
- Integrate Motor control to interface with motor-daemon for faster response.
- Add auto tracking to PTZ camera and allows turning on white light when motion is detected.
- Add seperate controls in WebUI to change sensors setting individually(or both).
- Integrate daynight sensing to Prudynt-T.
- Add variable digital zoom for each sensor, up to 2.5x. The zoom can be done in multi-levels.

<img width="458" height="535" alt="config-streamer-dual" src="https://github.com/user-attachments/assets/e96c9db7-0013-4a9f-bd07-2b4980a8556b" />

![service-motion-map](https://github.com/user-attachments/assets/1d25a4b6-20cd-46fe-a0c5-3ee2d687f9d0)

