
# Muscle Shirt

A shirt that shows muscle activations as you workout with LED bar graphs showing strain level. Also the colors change from green to red showing that your 'pump' levels have peaked and that muscle site has been suitablyt worked.


## Power Procedures

We burnt a board by not foolowing the correct powering sequences. 

1. Plug in positive ground.
2. Plug in positve +9V.
3. Plug in negative ground.
4. Plug in negative -9V.
5. Wait for at least two blinking red LEDs on the STM32.
6. Plug in the micro-USB for debugging over USB.

**Powering down must be done in exactly the opposite order. _DO NOT. I REPEAT DO NOT_, turn off the positive power while the negative is still plugged in. The negative power will burn the board through its ground. Also do not turn off positive power while still plugged into USB. The STM32L432KC selects its power source based on the first thing it connects to. Removing main power while still plugged into USB will cause dangerous power draw through the USB and burn the board. This ordering for USB during powering is mentioned in the documentation for the board as an afterthought. _PAY CAREFUL ATTENTION TO BATTERY LIFE_ if the negative side dies it takes the board with it.**


## Filter Testing Points

The TL074 are 4 op-amps in one. Starting in the top leftcorner and going clockwise the filters go HP->LP->Rectifiers(2)->Divider. When you see a resistor cross either accross the chip or over the negative voltaqge (green line pin, then you know you are looking at the output for the next phase. The Sallen-Key cutoff characteristics are determined based on the filter characteristics shown in the _filter(I&II).png_.The divider stepping down from +9V to the +5V for the ADC are one resistor off the output pin. For clarity, the filter headers from the top go input (blue), output (purple), +power (red), ground (black), then accross the chip -power (green).

## Pin Outs

The Arduino pin labels that appear on the Nucleo Board are shown on the right hand side, while the functional pin labels appear on the left. This chart makes connections easier. (although they are custom labelled on the .ioc file)

MUX_AO ->           D4

MUX_A1 ->           A5

MUX_A2 ->           D11

MUX_A3 ->           GND

MUX_EN ->           D3

Screen_SDA          D0

Screen_SCL ->       D1

LED_Ctrl ->         D9

EMG_ADC_IN ->       A3

Strain1_ADC_IN ->   D6

Strain2_ADC_IN ->   D4

Button_IN ->        D2  (3v3 Powers, Any GND GNDs)
## Header Designations

There is a file _header.png_ which shows the header slots for each sodered protoboard indicating which pin goes where. As a general rule we have colorized the cables such that:

+9 = (red)

-9 = (green)

GND = (black)

filter_in = (blue)

filter_out = (purple)

mux_addrs = (yellow)

mux_en = (grey)

adc_in = (brown)

led_ctrl = (white)

screen_i2c = (purple)

reset_button = (orange)




## Additional Notes of Caution

Make sure that the interrupt for both the button and the debouncing of the button is of a priority lower (meaning higher numerically) than the DMA interrupt. If the DMA transfer for updating the LED's is interrupted mid stream then a DMA_done flag is never set and the LED's stall out on just draining high current from the battery.

If the .ioc gets updated and you generate code make sure to go to the generated ADC section and removed the portions notated by the TODO comments. Also reduce the number of conversions to 1. We have implemented functions above that handle that code region that handle switching the ADC from the multiplexed ADC input for the EMG and the two strain (flex sensor inputs). We do not want the default scan mode switching that it tries to setup.

(From Demo Day)The battery packs we purchased have faulty springs that can mean all or a fraction of the AA batteries inside are disconnected. Make sure to stretch out the springs and verify voltage for both +9 and -9 before powering the system (FOLLOWING THE CORRECT PROTOCOL).
