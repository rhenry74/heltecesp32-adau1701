# heltecesp32-adau1701

Using Heltec ESP32 MCU to control an ADAU1701 audio DSP via I2C.

The ESP32 can control volume, bass, treble, midrange, mono, mute, subwoofer crossover frequency and color (bypass) by both monitoring hardware and over a web interface.

It does "safe write" to the DSP. If safe write is not use you will get nasty pops!

The Analog Devices - SigmaStudio project is included and the addresses of the DCs are mapped in the code. 

Hardware needed would include 5 rotary encoders. This is coded to used the Heltec non-lora ESP32 dev boad that has on-built OLED display. Momentary switches are needed for a few inputs. Many rotary encoders are also push puttons. Or the whole rig can be controled over the web. 

I could never find a way to do a "safe read" from the DSP without pops. So the MCU only ever writes to the DSP. For this reason it saves the settings for all the various controls on EEPROM. Settings persist when the power is cycled... nice. However, powering up on an EEPROM that has not been initialized with 'reasonable' parameters can cause bad things to happen, espesially if the DSP is connected to an amplifier. This is where howto-eeprom-init comes into play. Run it once before first upload of i2c-adau1701-howto.

