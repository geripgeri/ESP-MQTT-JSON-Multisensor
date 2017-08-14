# ESP MQTT JSON Multisensor

Based on https://github.com/bruhautomation/ESP-MQTT-JSON-Multisensor

This project shows a super easy way to get started with your own DIY Reed switch to use with [Home Assistant](https://home-assistant.io/), a sick, open-source Home Automation platform that can do just about anything.

Bonus, this project requires **no soldering** and **no breadboards** - just header wires and the development board! 

Video Tutorial - https://youtu.be/jpjfVc-9IrQ

The code covered in this repository utilizies Home Assistant's [MQTT Binary Sensor Component](https://home-assistant.io/components/binary_sensor.mqtt/) and a [NodeMCU ESP8266](http://geni.us/cpmi) development board.

### Supported Features Include
- **Reed switch** Door/Windows sensor
- **Over-the-Air (OTA)** upload from the ArduinoIDE


#### OTA Uploading
This code also supports remote uploading to the ESP8266 using Arduino's OTA library. To utilize this, you'll need to first upload the sketch using the traditional USB method. However, if you need to update your code after that, your WIFI-connected ESP chip should show up as an option under Tools -> Port -> Porch at your.ip.address.xxx. More information on OTA uploading can be found [here](http://esp8266.github.io/Arduino/versions/2.0.0/doc/ota_updates/ota_updates.html). Note: You cannot access the serial monitor over WIFI at this point.  


### Parts List

**Amazon Prime (fast shipping)**
- [NodeMCU 1.0](http://geni.us/cpmi)
- [Reed Switch](http://a.co/1PHxdQI )



### Wiring Diagram
![wiring diagram](https://github.com/geripgeri/ESP-MQTT-JSON-ReedSwitch-Sensor/raw/master/wiring_diagram.png "Wiring Diagram")


### Home Assistant Service Examples
Besides using the card in Home Assistant's user interface, you can also use the Services tool to control the light using the light.turn_on and light.turn_off services. This will let you play with the parameters you can call later in automations or scripts. 

Door status
```
{
"door": true
}
```
