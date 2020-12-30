# ESP32-web-template-sd

**_ESP32 + HTTP server + websockets + Bootstrap + Husarnet + configuration on SD. All useful technologies for creating internet controlled devices in one_**

This project template is a good base for creating internet controled devices with a web UI. Here main points:

**I. CONFIGURATION USING SD CARD**

One of the biggest problems in making connected things is the first configuration. Opening IDE just to change network credentials or hosted website is uncomfortable and takes to much time. Other options would be dedicated mobile app that you will use probably only once (one more?) or CLI over serial to your things (it's so easy to make a typo and redo the whole configuration again).

I tested every option and the most convenient for me is editing configuration file placed on the SD card. Configuration file uses JSON, is very simple and looks like this:

```
{
	"husarnet":{
		"hostname":"esp32template",	
		"joincode":"fc94:b01d:1803:8dd8:b293:5c7d:7639:932a/xxxxxxxxxxxxxxxxxxxxxx"
	},
	"wifi":[
		{
			"ssid":"mywifinet123",
			"pass":"qwerty"
		},
		{
			"ssid":"officenet",
			"pass":"admin1"
		},
		{
			"ssid":"iPhone(Johny)",
			"pass":"12345"
		}
	]
}
```
There are two blocks:
- Husarnet credentials - Husarnet allows you to access your things over the internet just like they were in the same LAN network
- WiFi credentials - place one or more SSID-Password pairs, so you will not need to change a configuration if you move your thing somewhere else

**II. HTML FILE IS NOT HARDCODED**

Editing HTML files as a C/C++ char tables is not very convenient. This is why I moved HTML file to the SD card. The whole interface between web page and the hardware is through websockets, so you do not need to parse HTTP requests and your code is much cleaner. Websockets are also much faster than HTTP requests.

**III. ACCESS OVER THE INTERNET USING VPN**

There are a few benefits:
- **very easy configuration of your devices** (you just need to provide a joincode and think of a name for your thing). Just take a look at JSON. Joincode is generated only once and you can use it to connect together your computer and ESP32 devices.
- **access your device like it was in your LAN**. Website is hosted on ESP32 and you access that website directly over the internet. Husarnet gives only transparent connectivity layer working both in you LAN and over the internet the same way. Husarnet is basically a VPN.
- **low latency**. Your computer (with Husarnet client installed) and ESP32 are connected peer-to-peer. Husarnet infrastructure only help your devices to find each other over the internet and is not used to forward user data after P2P connection is established. If your devices are in the same LAN network you will control it just over LAN.

-- 

A demo is really basic:

- control a LED connected to ESP32 by using a button in web UI.
- ESP32 sends a counter value every 100ms.
- ESP32 sends a button state to change a color of the dot in web UI.

**A. HARDWARE**

Connect button to between G22 pin and GND.

Connect LED with a serial resistor between G16 pin and GND.

Connect SD card module to you ESP32 board: if you already have ESP32 dev board with SD card slot, other pins might be used, so you would need to modify also software part.

Interface between my ESP32 dev board and SD card module is as follows:
```
SD CARD <-> ESP32 dev board

CS      <-> G22           
SCK     <-> G18
MOSI    <-> G23
MISO    <-> G19
VCC     <-> 5V out
GND     <-> GND
```

Format SD card to FAT16/FAT32.

Copy these files to a SD card just formated:
- setting.js
- index.htm

Modify settings.js file by providing your own WiFi network credentials and Husarnet credentials. 

To find your joincode:
a) register at https://app.husarnet.com
b) create a new network or choose existing one
c) in the chosen network click **Add element** button and go to "join code" tab

Save your files and attach the SD card to your ESP32.

**B. SOFTWARE:**

To run the project, open Arduino IDE and follow these steps:

**1. Install Husarnet package for ESP32:**

- open `File -> Preferences`
- in a field **Additional Board Manager URLs** add this link: `https://github.com/husarnet/arduino-esp32/releases/download/1.0.4-1/package_esp32_index.json`
- open `Tools -> Board: ... -> Boards Manager ...`
- Search for `esp32-husarnet by Husarion`
- Click Install button

Please note, that we include here a modified fork (mainly IPv6 support related changes) of official Arduino core for ESP32 - https://github.com/husarnet/arduino-esp32 . If you had installed the original version before, it is recommended to remove all others Arduino cores for ESP32 that you had in your system.

**2. Select ESP32 dev board:**

- open `Tools -> Board`
- select **_ESP32 Dev Module_** under "ESP32 Arduino" section

**3. Install ArduinoJson library:**

- open `Tools -> Manage Libraries...`
- search for `ArduinoJson`
- select version `5.13.3`
- click install button

**4. Install arduinoWebSockets library (Husarnet fork):**

- download https://github.com/husarnet/arduinoWebSockets as a ZIP file (this is Husarnet compatible fork of arduinoWebSockets by Links2004 (Markus) )
- open `Sketch -> Include Library -> Add .ZIP Library ... `
- choose `arduinoWebSockets-master.zip` file that you just downloaded and click open button


**5. Program ESP32 board:**

- Open **ESP32-web-template-sd.ino** project
- upload project to your ESP32 board.

**6. Open WebUI:**
There are two options:

1. log into your account at https://app.husarnet.com, find `esp32template` device that you just connected and click `web UI` button. You can also click `esp32template` element to open "Element settings" and select `Make the Web UI public` if you want to have a publically available address. In such a scenerio Husarnet proxy servers are used to provide you a web UI.
2. P2P option - add your laptop to the same Husarnet network as ESP32 board. In that scenerio no proxy servers are used and you connect to ESP32 with a very low latency directly with no port forwarding on your router! Currenly only Linux client is available, so open your Linux terminal and type:

- `$ curl https://install.husarnet.com/install.sh | sudo bash` to install Husarnet.
- `$ husarnet join XXXXXXXXXXXXXXXXXXXXXXX mylaptop` replace XXX...X with your own `join code`. 

To find your join code:
a) register at https://app.husarnet.com
b) create a new network or choose existing one
c) in the chosen network click **Add element** button and go to "join code" tab

At this stage your ESP32 and your laptop are in the same VLAN network. The best hostname support is in Mozilla Firefox web browser (other browsers also work, but you have to use IPv6 address of your device that you will find at https://app.husarnet.com) and type:
`http://esp32template:8000`
You should see a web UI to controll your ESP32 now.
