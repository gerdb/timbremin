![timbremin](pics/timbremin_logo.png "logo timbremin")

<br><br><br><br><br>
# !! Project under construction !!
<br><br><br><br><br><br><br><br>


### An easy to build Theremin for only 50$

* Only an STM32 evaluation board, 3 oscillators and some knobs and switches needed.
* Excellent audio quality: 16/24bit audio DAC with headphone amplifier
* Can load waveforms with Theremin sound directly from an USB stick
* Very fast autotune: 1sec


![timbremin](pics/timbremin.png "timbremin")

# Hardware
The STM32F407G-DISC1 evaluation board can be used.
So you need only 3 additional oscillators and some potentiometers and switches.


# The LC colpitts oscillators

### Schematic
![tinnitus](pics/osc_sch.png "timbremin oscillator schematic")

### Layout
The oscillators can be built up on a breadboard.
With THD components:

![timbremin](pics/osc_THD.png "timbremin oscillator built on a breadboard")

Or with SMD components:

![timbremin](pics/osc_SMD.png "timbremin oscillators built on a breadboard")


### Bill of material
| Component | Volume 1  | Volume 2  | Pitch     |                   |
| --------- | --------- | --------- | --------- |------------------ |
| C1        | 1µF       | 1µF       | 1µF       | ceramic capacitor |
| C2        | 470pF     | 470pF     | 470pF     | ceramic capacitor |
| C3        | 33pF      | **68pF**  | 33pF      | ceramic capacitor |
| C4        | 2.2nF     | 2.2nF     | 2.2nF     | ceramic capacitor |
| R1        | 10R       | 10R       | 10R       | resistor 1%       |
| R2        | 22k       | 22k       | **1Meg**  | resistor 1%       |
| R3        | 100R      | 100R      | 100R      | resistor 1%       |
| IC1       | CD4011UBE | CD4011UBE | CD4011UBE | Quad NAND         |


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
Connect an external button to:

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


## Programming software for STM32
You find the binary file in the "Binary" folder. Download the [tinnitus project as ZIP](https://github.com/gerdb/tinnitus32/archive/master.zip)

Program the STM32 board with the [STM32CUBEPROG](http://www.st.com/content/st_com/en/products/development-tools/software-development-tools/stm32-software-development-tools/stm32-programmers/stm32cubeprog.html) tool.

## Case and front panel
A wooden wine box or a bamboo box for the kitchen (both about 10$) can be used as a case for the timbremin.

![Wooden wine box](pics/winebox.jpg "Wooden wine box")

The cheapest way for a professional looking front panel is to use the service of a sign manufacturer. For example:
https://www.digitaldruck-fabrik.de/werbeschilder/hart-pvc-polystyrolplatte.aspx

Price of a 380x110mm front panel: 5$

You can use the template under hardware/Frontpanel, but you have to adapt the size of the front panel file to your case. Use eg. https://inkscape.org

![Front panel](pics/frontpanel.png "Front panel")

