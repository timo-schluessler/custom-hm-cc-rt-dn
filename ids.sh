#!/bin/sh

# 9A 45 A0  HMCCRTDN00  Erstes Testthermostat


echo -ne "\x9a\x45\xa0HMCCRTDN00" | srec_cat - -binary -offset 0x1000 -o id.ihx -intel
stm8flash -p stm8l152r8 -c stlinkv2 -s eeprom -w id.ihx
rm id.ihx
