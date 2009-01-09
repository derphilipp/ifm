# Makefile for scr2ifm tests.

SRCS		= ../scr2ifm.pl
PROG		= perl $(SRCS) -i
DEPEND		= $(SRCS) Makefile

%.ifm:		%.scr %.cmd $(DEPEND)
		$(PROG) -c $*.cmd -o $@ $**.scr

%.ifm:		%1.scr %.cmd $(DEPEND)
		$(PROG) -c $*.cmd -o $@ $**.scr

%.ifm:		%.scr $(DEPEND)
		$(PROG) -o $@ $**.scr

%.ps:		%.ifm
		ifm -m -fps -o $@ $<

%.gv:		%.ps
		gv $<

clean:;		rm -f *.ifm *.ps
