#sdcc -mstm8 -lstm8 main.c --out-fmt-elf --debug -o main.elf

#sdcc -mstm8 main.c -c --debug --all-callee-saves --verbose --stack-auto --fverbose-asm --float-reent --no-peep
sdcc -mstm8 lcd.c -c --debug --opt-code-size --all-callee-saves --verbose --stack-auto --fverbose-asm --float-reent --no-peep
sdcc -mstm8 time.c -c --debug --opt-code-size --all-callee-saves --verbose --stack-auto --fverbose-asm --float-reent --no-peep
#sdcc -mstm8 spi.c -c --debug --opt-code-size --all-callee-saves --verbose --stack-auto --fverbose-asm --float-reent --no-peep

sdcc -mstm8 --out-fmt-elf --debug --all-callee-saves --verbose --stack-auto --no-peep -o main.elf main.c lcd.rel time.rel
#sdcc -mstm8 --out-fmt-elf --debug --opt-code-size --all-callee-saves --verbose --stack-auto --no-peep -o main.elf main.c lcd.rel time.rel

../install/bin/stm8-objcopy -j HOME -j GSINIT -j GSFINAL -j CODE -j INITIALIZER -I elf32-stm8 main.elf -O ihex main.ihx
../install/bin/stm8-objcopy -I ihex main.ihx -O binary main.bin
