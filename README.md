![timbremin](pics/timbremin_logo.png "logo timbremin")

### An easy to build Theremin for only 50$

* Only an STM32 evaluation board, 2 oscillators and some knobs and switches needed.
* Excellent audio quality: 16/24bit audio DAC with headphone amplifier
* Can load waveforms with Theremin sound directly from an USB stick
* Very fast autotune: 1sec


![timbremin](pics/timbremin.png "timbremin")

# Hardware
The STM32F407G-DISC1 evaluation board can be used.
So you need only 3 additional oscillators and some potentiometers and switches.


# The LC colpitts oscillators

### Schematic
![tinnitus](pics/timbremin_osc_sch.png "timbremin oscillator schematic")

### Layout
The oscillators can be built up on a breadboard:

![timbremin](pics/timbremin_osc_pcb_thd.png "timbremin oscillator built on a breadboard")
![timbremin](pics/oscillators.jpg "timbremin oscillators built on a breadboard")


### Bill of material
| Component     | Value        | Details                             |
| ------------- | ------------ |------------------------------------ |
| C1            | 22pF         | ceramic capacitor NP0 2.54mm        |
| C2            | 10pF         | ceramic capacitor NP0 2.54mm        |
| C3            | 1nF          | ceramic capacitor NP0 or X7R 2.54mm |
| C4            | 220pF        | ceramic capacitor NP0 2.54mm        |
| C5            | 1µF          | ceramic capacitor X7R or X5R 5.08mm |
| R1            | 100k         | resistor 1% size 0207               |
| R2            | 10R          | resistor 1% size 0207               |
| R3            | 1k           | resistor 1% size 0207               |
| L1            | 2.5, 3, 4mH  | air coil, see below                 |
| T1            | BC557C       | TO92                                |


L1 should be an air-coil with 2.5mH, 3mH and 4mH.
Eg. x windings of 0.15mm wire on a core with 50mm diameter.
Picture below shows all 3 coils with different number of windings.
![timbremin air coils](pics/coils.jpg "timbremin air coils")

Use http://hamwaves.com/antennas/inductance.html for other diameters.




## Pin maps

| Name       | PIN Name | Connector | Description                                    |
| ---------- | -------- | --------- | ---------------------------------------------- |
| GND        | GND      | P1        | Ground for oscillators and potentiometers      |
| VDD        | VDD      | P1        | 3V supply for oscillators and potentiometers   |
| PITCH_OSC  | PE9      | P1        | Signal from pitch oscillator                   |
| VOLUME_OSC | PE11     | P1        | Signal from volume oscillator                  |
| ANALOG_1   | PA1      | P1        | Analog input from volume potentiometer         |
| ANALOG_2   | PA2      | P1        | Analog input from zoom volume potentiometer    |
| ANALOG_3   | PA3      | P1        | Analog input from shift pitch potentiometer    |
| ANALOG_4   | PC4      | P1        | Analog input from zoom pitch potentiometer     |
| ANALOG_5   | PC5      | P1        | Analog input from waveform potentiometer       |

Connect an extra 100nF capacitor under each potentiometer from the potentiometer output to GND.

## Auto-tune
The blue button on the STM32 discovery board starts the Auto-Tune procedure.

![auto tune button](pics/auto_tune.png "auto tune")

You can also use an additional button and connect it to:

| Name       | PIN Name | Connector | Description                   |
| ---------- | -------- | --------- | ----------------------------- |
| VDD        | VDD      | P1        | 3V supply auto-tune button    |
| Auto-tune  | PA0      | P1        | Signal from auto-tune button  |

## Pitch LED display
Optional display to show the current played note one a piano like display.

![LED display](pics/led_display.png "LED display")

Connect 3mm LEDs with a series resistor of 100R to these PINs and connect all
cathodes of to GND:

![LED display](pics/leds.png "LED display")

| Name         | PIN Name | Connector | Description                                    |
| ------------ | -------- | --------- | ------------------------ |
| PITCH LED 0  | PE12     | P1        | Note c                   |
| PITCH LED 1  | PE14     | P1        | Note cis                 |
| PITCH LED 2  | PE15     | P1        | Note d                   |
| PITCH LED 3  | PB11     | P1        | Note dis                 |
| PITCH LED 4  | PB12     | P1        | Note e                   |
| PITCH LED 5  | PB13     | P1        | Note f                   |
| PITCH LED 6  | PB14     | P1        | Note fis                 |
| PITCH LED 7  | PB15     | P1        | Note g                   |
| PITCH LED 8  | PD8      | P1        | Note gis                 |
| PITCH LED 9  | PD9      | P1        | Note a                   |
| PITCH LED 10 | PD10     | P1        | Note b                   |
| PITCH LED 11 | PD11     | P1        | Note h                   |


## Power supply
The STM32 discovery board must be supplied by 5V (100mA). It is importand that the GND is connected to earth (PE). If a normal wall adapter is used, there must be an extra connection to earth (PE).

The best power supply would be a linear one like this: https://github.com/gerdb/tinnitus32/wiki/Power-supply


## USB-stick with sound file
It's possible to store a \*.WAV or a \*.C file on an USB-stick and use it as sound source for the theremin.
#### WAV file
The file name must be **WAV1.WAV** and it must contain 1 periode of a sound sample. File format must be WAV (Microsoft) and the data format signed 16 bit PCM with only one mono track. Use Audacity to export it. https://www.audacityteam.org/ 
#### C file
The file name must be **WAV1.C** and it must contain 1 periode of a sound sample. File format must be the same as the Audio Wavetables from the Open.Theremin V3 project. http://www.gaudi.ch/OpenTheremin/index.php/download/audio-wavetables 


## Programming software for STM32
You find the binary file in the "Binary" folder. Download the [tinnitus project as ZIP](https://github.com/gerdb/tinnitus32/archive/master.zip)

Program the STM32 board with the [STM32CUBEPROG](http://www.st.com/content/st_com/en/products/development-tools/software-development-tools/stm32-software-development-tools/stm32-programmers/stm32cubeprog.html) tool.

## Case and front panel
As case a wooden wine box could be used.

![Wooden wine box](pics/wine_box.jpg "Wooden wine box")

The cheapest way for a professional looking front panel would be to use the service of a sign manufacturer. For example:
https://www.digitaldruck-fabrik.de/werbeschilder/hart-pvc-polystyrolplatte.aspx

Price of a 380x110mm front panel is about 4€ or 5$

You can use the template under hardware/Frontpanel, but you have to adapt the size of the front panel file to your wine box. Use eg. https://inkscape.org

![Front panel](pics/frontpanel.png "Front panel")

