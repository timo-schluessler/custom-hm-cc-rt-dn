#sdcc -mstm8 -lstm8 main.c --out-fmt-elf --debug -o main.elf

#sdcc -mstm8 main.c -c --debug --all-callee-saves --verbose --stack-auto --fverbose-asm --float-reent --no-peep
sdcc -mstm8 lcd.c -c --debug --opt-code-size --all-callee-saves --verbose --stack-auto --fverbose-asm --float-reent --no-peep
sdcc -mstm8 time.c -c --debug --opt-code-size --all-callee-saves --verbose --stack-auto --fverbose-asm --float-reent --no-peep
#sdcc -mstm8 spi.c -c --debug --opt-code-size --all-callee-saves --verbose --stack-auto --fverbose-asm --float-reent --no-peep

sdcc -mstm8 --out-fmt-elf --code-loc 0x8000 --debug --all-callee-saves --verbose --stack-auto --no-peep -o main.elf main.c lcd.rel time.rel
if [ $? -ne 0 ]; then
	exit
fi
#sdcc -mstm8 --out-fmt-elf --debug --opt-code-size --all-callee-saves --verbose --stack-auto --no-peep -o main.elf main.c lcd.rel time.rel

../install/bin/stm8-objcopy -j HOME -j GSINIT -j GSFINAL -j CODE -j INITIALIZER -I elf32-stm8 main.elf -O ihex main.ihx
../install/bin/stm8-objcopy -I ihex main.ihx -O binary main.bin

OFFSET=0x8000
# use this to insert crc.
# srec_cat main.ihx -intel -offset -0xa000 -fill 0xFF 0x0 0xdffe -CRC16_Big_Endian 0xdffe -POLYnomial ibm -o main.tmp -binary

# use this without crc and faster updates
LAST=$(srec_info main.ihx -intel -offset -${OFFSET} | tail -n 1 | sed -e 's/^.* - //g')
LAST=$(echo "obase=16; ibase=16; ($LAST + 4 + 7F) / 80 * 80;" | bc -q) # add 1 to get length, add 1 for 00 before crc, add 2 for crc, round up to 128 bytes block size
CODE_END=$(echo "obase=16; ibase=16; ${LAST} - 3;" | bc -q)
CRC=$(echo "obase=16; ibase=16; ${LAST} - 2;" | bc -q)
echo last is now $LAST
srec_cat '(' main.ihx -intel -offset -${OFFSET} -fill 0xFF 0 0x${CODE_END} -generate 0x${CODE_END} 0x${CRC} -constant 0x55 ')' -crc16_big_endian 0x${CRC} -polynomial ibm -o main.tmp -binary
php bin2eq3.php main.tmp main.eq3 128
#rm main.tmp

if [ $# -ne 0 ]; then
	#../install/bin/stm8-gdb main.elf -ex "target remote :3333" -ex "mon reset halt" -ex "load" -ex "jump *0xa007"
	../install/bin/stm8-gdb main.elf -ex "target remote :3333" -ex "mon reset halt" -ex "load" -ex "c"
fi
