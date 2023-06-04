| <img src="./assets/RAK-Whirls.png" alt="RAKWireless"> | <img src="./assets/WisBlock-logo.png" alt="WisBlock"> | <img src="./assets/rakstar.jpg" alt="RAKstar" > |    
| :-: | :-: | :-: |     

# Crazy idea to monitor your washing machine and know when it has finished.

Our washing machine is in the garage and often we forgot that we had it started, that it already finished and we should hang our laundry for drying.
I was thinking about a monitoring system to know when the washing machine has finished its job and had some ideas:    
(1) measure the current supplied to the machine     
(2) put a sensor on the power light of the machine    
(3) (that's the crazy idea) measure the vibrations of the machine and when it is no longer shaking, we know it has finished    

I went for the idea (3), because it is the easiest to add to the washing machine without cutting any cables or put light sensors on it. 

----

The application offers at the moment to setup two parameters via AT commands:

**`ATC+SENDINT=xxxxx`** => Allows to setup the heartbeat (I am alive) sending interval in xxxxx seconds.

**`ATC+TOUT=xxxx`** => Allows to setup the timeout for "off" status in milliseconds. For the washing machine example, I set this to 30 seconds before the application reports "inactivity".

### ⚠️ TODO:
Add setup for the sensitivity of the vibration. At the moment this is fixed to 1/8th of the range of 2G. This makes it very sensitive.

----

# Code sections

The code is based on the [WisBlock-API-V2](https://github.com/beegee-tokyo/WisBlock-API-V2).     
Targeting low power consumption, WisBlock-API-V2 for RAKwireless WisBlock Core modules takes care of all the LoRa P2P, LoRaWAN, BLE, AT command functionality. You can concentrate on your application and leave the rest to the API. It is made as a companion to the [SX126x-Arduino LoRaWAN library](https://github.com/beegee-tokyo/SX126x-Arduino) :arrow_upper_right:    
It requires some rethinking about Arduino applications, because you have no `setup()` or `loop()` function. Instead everything is event driven. The MCU is sleeping until it needs to take actions. This can be a LoRaWAN event, an AT command received over the USB port or an application event, e.g. an interrupt coming from a sensor. 

This approach makes it easy to create applications designed for low power usage. During sleep the WisBlock Base + WisBlock Core RAK4631 consume only 40uA.

In addition the API offers two options to setup LoRa P2P / LoRaWAN settings without the need to hard-code them into the source codes.    
- AT Commands => [AT-Command Manual](https://docs.rakwireless.com/RUI3/Serial-Operating-Modes/AT-Command-Manual/https://docs.rakwireless.com/RUI3/Serial-Operating-Modes/AT-Command-Manual/) ↗️
- WisToolBox ==> [WisToolbox](https://docs.rakwireless.com/Product-Categories/Software-Tools/WisToolBox) ↗️

----

# Get WisBlock devices

Get RAKwireless WisBlock modules, including Base Boards, Core modules, Sensor and IO modules from RAKwireless [store](https://store.rakwireless.com/collections/wisblock)

----

# LoRa® is a registered trademark or service mark of Semtech Corporation or its affiliates. 

----

# LoRaWAN® is a licensed mark.