sdcc -mstm8 --out-fmt-elf --debug --all-callee-saves --verbose --stack-auto --no-peep -o bootloader.elf bootloader.c && \
../install/bin/stm8-gdb bootloader.elf -ex "target remote :3333" -ex "mon reset halt" -ex "load" -ex "c"
