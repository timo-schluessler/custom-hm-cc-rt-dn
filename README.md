# custom-hm-cc-rt-dn
Custom firmware for ELV Homematic HM-CC-RT-DN radiator thermostat. Includes fully working and Homematic-compatible OTA bootloader.

## Radio
Fully functional and Homematic-compatible.

## Display
Fully functional.

## Buttons and Wheel
Fully functional.

## Motor control
Partially functional. Could be improved. The original firmware seems to adjust the PWM duty cycle to battery voltage.

## Temperature control
Not implemented. The original goal was to do the control centralized (e.g. in FHEM) and send the desired motor position to the thermostat.

## Bootloader
Fully functional and Homematic-compatible - at least with respect to the radio protocol and host software. ELV Homematic uses encrypted firmware files which are *NOT* compatible with this bootloader!
