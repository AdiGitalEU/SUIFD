# SUIFD
Simple UI for Duet Wifi

A simple UI for Duet WiFi board (https://www.duet3d.com) It's meant for my personal use but was asked to share the code. I'm not fluent in wriging a sharable code. Please excuse if I broke conventions etc.
Please let me know if I violated any licenses in the few libraries I used and I'll happily fix it.

Warning!
As it was designed for my onw purposes there are no safety measures other than in Duet firmware. Use it on your own risk. The communication does not implement checksum so it may introduce errors.

The UI is based on ESP8266 and ILI9341. It communicates with Duet via serial port and connects to PanelDue. Nearly everything is hardcoded. The graphics can be replaced by your own but if you change sizes you need to adapt the coordinates in code.

Before uploading I tested the code and it works with the libraries versions (listed in code). I suggest testing the display first using some examples from the library.

Step one would be to upload the Data folder to the internal SPIFFS. Use the Arduino IDE Tools/ESP8266 Sketch Data Upload. Then uploading the sketch should give working UI even when disconnected from Duet (without the values of course).

Connections between ESP and the display:
D0 - Touch CS
D1 - Touch IRQ
D3 - DC
D5 - TFT SCK & Touch SCK
D6 - TFT MISO & Touch MISO
D7 - TFT MOSI & Touch MOSI 
D8 - TFT CS
Rst - Rst

TX & RX -> PanelDUE port.

I also used 3.3V beeper between 3.3V and D2
