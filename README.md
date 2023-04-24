
# Muscle Shirt

A shirt that shows muscle activations as you workout with LED bar graphs showing strain level. Also the colors change from green to red showing that your 'pump' levels have peaked and that muscle site has been suitablyt worked.


## Power Procedures

We burnt a board by not following the correct powering sequences. 

1. Plug in positive ground.
2. Plug in positve +9V.
3. Plug in negative ground.
4. Plug in negative -9V.
5. Wait for at least two blinking red LEDs on the STM32.
6. Plug in the micro-USB for debugging over USB.

**Powering down must be done in exactly the opposite order. _DO NOT. I REPEAT DO NOT_, turn off the positive power while the negative is still plugged in. The negative power will burn the board through its ground. Also do not turn off positive power while still plugged into USB. The STM32L432KC selects its power source based on the first thing it connects to. Removing main power while still plugged into USB will cause dangerous power draw through the USB and burn the board. This ordering for USB during powering is mentioned in the documentation for the board as an afterthought. _PAY CAREFUL ATTENTION TO BATTERY LIFE_ if the negative side dies it takes the board with it.**


## Filter Testing Points

The TL074 are 4 op-amps in one. Starting in the top leftcorner and going clockwise the filters go HP->LP->Rectifiers(2)->Divider. When you see a resistor cross either accross the chip or over the negative voltaqge (green line pin, then you know you are looking at the output for the next phase. The Sallen-Key cutoff characteristics are determined based on the filter characteristics shown in the _filter(I&II).png_.The divider stepping down from +9V to the +5V for the ADC are one resistor off the output pin. For clarity, the filter headers from the top go input (blue), output (purple), +power (red), ground (black), then accross the chip -power (green).
