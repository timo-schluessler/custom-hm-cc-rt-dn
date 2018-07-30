#ifndef DELAY_INCLUDED
#define DELAY_INCLUDED

#define F_CPU 125000UL
#define T_COUNT(x) ((( F_CPU * x / 1000000UL )-5)/5)

static inline void _delay_cycl( unsigned short __ticks )
{
	__asm__("nop\n nop\n"); 
	do {    
		__ticks--;
	} while ( __ticks );
	__asm__("nop\n");
}

static inline void _delay_us( unsigned short __us )
{
	_delay_cycl( (unsigned short) T_COUNT(__us) );
}

static inline void _delay_ms( unsigned short __ms )
{
	while ( __ms-- )
	{
		_delay_us( 1000 );
	}
}

#endif
