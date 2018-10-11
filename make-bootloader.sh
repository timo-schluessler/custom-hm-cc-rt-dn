sdcc -mstm8 --std-sdcc11 --out-fmt-elf --code-loc 0x16000 --debug --all-callee-saves --verbose --stack-auto --noinduction --use-non-free -o bootloader.elf bootloader.c || exit 1

# copy bootloader reset vector to 0x8000
../install/bin/stm8-objcopy -j HOME -j GSINIT -j GSFINAL -j CODE -j INITIALIZER -I elf32-stm8 bootloader.elf -O ihex bootloader.hex
srec_cat bootloader.hex -intel bootloader.hex -intel -offset -0xe000 -crop 0x8000 0x8004 -o bootloader-full.hex -intel

if [ $# -ne 0 ]; then
	#stm8flash -c stlinkv2 -p stm8l152?8 -s flash -w bootloader-full.hex # use this once to also write the real reset vector at 0x8000
	stm8flash -c stlinkv2 -p stm8l152?8 -s 0x16000 -v bootloader.hex
	../install/bin/stm8-gdb bootloader.elf -ex "target remote :3333" -ex "mon reset halt" -ex "c"
fi
