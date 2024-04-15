NUM_PROC=10


MPICC=mpicxx
MPIRUN=mpirun
MPIFFLAGS= -O3
MPIRUN_FLAGS= -n $(NUM_PROC) --map-by :OVERSUBSCRIBE

all: main.out
	$(MPIRUN) $(MPIRUN_FLAGS) main.out

%.o: %.cpp
	$(MPICC) -c $(MPIFFLAGS) $< -o $@

%.out: %.o
	$(MPICC) $(MPIFFLAGS) $< -o $@

clean:
	rm -rf *.o *.out
