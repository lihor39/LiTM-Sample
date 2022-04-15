include parallelDefs

COMMON = IO.h parseCommandLine.h parallel.h runTests.py sequence.h utils.h graph.h graphUtils.h blockRadixSort.h deterministicHash.h transpose.h graphIO.h

%.o : %.C $(COMMON)
	$(PCC) $(PCFLAGS) -c $< -o $@ -g

CHECKFILES = MISCheck.o

MISCheck : $(CHECKFILES)
	$(PCC) -fcilkplus -lcilkrts -mcx16 -O2 -DCILKP -DMCX16 $(PLFLAGS) -o $@ $(CHECKFILES)

$(COMMON) :
	ln -s ../../common/$@ .

clean :
	rm -f MISCheck *.o

cleansrc :
	make -s clean
	rm -f $(COMMON) *.pyc
