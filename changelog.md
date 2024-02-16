# Material Plane Sensor Changelog
### Sensor v3.1.0 - 17-02-2024
Additions:
<ul>
<li>Firmware works on the DIY sensors again</li>
<li>Firmware can now be installed on ESP32-S3 boards, including the TinyS3, which is an alternative to the TinyPICO for the full diy sensor</li>
<li>Sensor can now be calibrated from the webserver</li>
<li>Added ability to do OTA (WiFi) updates through Material Companion and Arduino IDE</li>
<li>If sensor can't connect to a WiFi network it will host its own network for configuration purposes</li>
</ul>

Fixes:
<ul>
<li>Material Companion can now read sensor configuration over USB even if 'Serial Output' is disabled</li>
<li>Sensor will wait transmitting coordinate data if no base ID has been resolved, but decoding is in progress</li>
</ul>

Other:
<ul>
<li>Improved the sensitivity of the DIY sensor</li>
</ul>

### Sensor v3.0.3 - 04-10-2023
Additions:
<ul>
<li>Implemented 'multi-point calibration' and 'offset calibration'</li>
</ul>

Fixes:
<ul>
<li>'Debug Enable' and 'Serial Output' settings were not saved after a restart</li>
<li>Some sensor settings were not properly restored after calibration</li>
<li>Fixed sensor crash that would sometimes occur when scanning for WiFi stations</li>
</ul>


### Sensor v3.0.2 - 30-09-2023
Initial release for the production sensor.

### Previous Releases
Previous releases can be found <a href="https://github.com/MaterialFoundry/MaterialPlane_Hardware">here</a>.