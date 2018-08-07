#ifndef DELAY_INCLUDED
#define DELAY_INCLUDED

#define T_COUNT(x) ((( F_CPU * x / 1000000UL )-5)/5)

static void delay_cycl( unsigned short ticks )
{
	__asm__("nop\n nop\n"); 
	do {    
		ticks--;
	} while (ticks);
	__asm__("nop\n");
}

static void delay_us( unsigned short us )
{
	delay_cycl( (unsigned short) T_COUNT(us) );
}

static void delay_ms( unsigned short ms )
{
	while (ms--)
	{
		delay_us(1000);
	}
}

#endif
