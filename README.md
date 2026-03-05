3/4/2026

This project is based on the wonderful work by team of thingino-firware and Prudynt-T. Some features been added to the Prudynt-T and WebUI for my own experiment.

The code was based on the “master+eebcb12” build with Oct. 2025 JASON WebUI. And, it has only been tested on <szt_ct211_t23n_gc1084_dual_atbm6012bx> dual sensors camera, which is not a currently supported platform, so, it is nowhere near completed.

Features:

- Dual sensor support for the T23 based camera. The sensor is switched at T23 MIPI interface with a MIPI switch and feed to two seperate stream path through encoders. The is different with the front end switching done in the earlier project, and this is how the Ingenic based design in comercial deployment. The work still in progress as there are issues at image processing area need to be addressed.
- Add dual-sensor/dual-stream RTSP/JPEG streaming and dual-view to WebUI.
- Integrate GPIO control into Prudynt for better access to the lights, sensor switch.
- Add Tiled/Map multi-ROIs to motion control. This allows detection of regions of interest only to avoid false alarm.
- Integrate Motor control to interface with motor-daemon for faster response.
- Add auto tracking to PTZ camera and turn on white light when motion is detected.
