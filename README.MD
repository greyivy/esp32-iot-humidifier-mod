# A simple ESP32 IOT humidifier mod

A proof-of-concept ESP32 (Heltec WiFi Kit 32) IOT humidifier using MQTT for communication. A custom PCB designed in Fritzing is included.

This is my first time writing C++ and designing a PCB.

## Why?

I was looking into humidifiers that could maintain a constant humidity level in my house to keep a bunch of houseplants happy. I didn't want to buy a new one, so I set out to mod an existing humidifier I was using.

My requirements:
- Accurate MQTT monitoring of humidity, temperature, and tank level compatible with [Home Assistant](https://www.home-assistant.io)
- Notifications when the tank is empty or the humidity drops too low
- A display showing this information
- Ability to sync with other IOT humidifiers to turn on/off as needed to maintain ideal humidity
- Compact and cute!

## How?

The humidifier I was modding was very basic: it had a potentiometer to control the output level and it knew whether the tank was full or empty with no level information. It operated at a weird voltage as well.

After a lot of experimentation with various parts, I ended up creating a simple octocoupler out of an LED and a binary photoresistor. I connected this to where the "tank empty" LED was. I developed a basic algorithm that learned approximately how long a tank would last until the "tank empty" LED turned on and used that to calculate the tank's level based on runtime and output level.

A relay was used to turn the humidifier on and off. While I would have loved to power the PCB from the humidifier's power supply, I decided to opt for a USB power supply to keep things simple.

I ended up going with an SHT21 humidity/temperature sensor because of its accuracy.

I've been using this setup for a few months now, running 24/7, and it works really well!

## Photos

<img src="https://user-images.githubusercontent.com/5335625/119880605-f1ff6180-bf1b-11eb-88dc-2b11bc892f85.jpg" width="600">


Top

![Top](https://user-images.githubusercontent.com/5335625/119880927-46a2dc80-bf1c-11eb-864b-256160d18dc0.png)

Bottom

![Bottom](https://user-images.githubusercontent.com/5335625/119881012-58847f80-bf1c-11eb-8d93-c0720a5354f0.png)

## Order PCB

<a href="https://oshpark.com/shared_projects/rhNz8zwd"><img src="https://oshpark.com/packs/media/images/badge-5f4e3bf4bf68f72ff88bd92e0089e9cf.png" alt="Order from OSH Park"></img></a>

## Bill of materials

TODO

## Issues

- [ ] I have been unable to get Fritzing to complete one of the connections on the PCB, so it must be soldered manually
- [ ] Due to the thermometer's close proximity to the development board, its measurements are skewed up due to heat output