# heltecesp32-adau1701
Using Heltec ESP32 MCU to control an ADAU1701 audio DSP via I2C.

The ESP32 can control volume, bass, treble, midrange, mono, mute, subwoofer crossover frequency and color (bypass) by both monitoring hardware and over a web interface.

It does "safe write" to the DSB. If safe write is not use you will get nasty pops!

The Analog Devices - SigmaStudio project is included and the addresses of the DCs are mapped in the code. 

Hardware needed would include 5 rotary encoders. This is coded to used the Heltec non-lora ESP32 dev boad that has on-built OLED display.
