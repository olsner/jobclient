CFLAGS = -Wall -Werror

PROGRAMS = jobclient jobforce jobcount

all: $(PROGRAMS)

clean::
	rm -f $(PROGRAMS)

$(PROGRAMS): jobclient.h

%: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)

TESTS = 10

test: $(PROGRAMS)
	@for (( t = 1; t <= $(TESTS); t++ )); do echo Running test$$t...; env -i make -j3 test$$t || break; echo; done

# Tests assume running with -j3 (and no concurrent activity), which means that
# each command actually runs with 2 job tokens available. One job is given
# implicitly (which then requires jobforce to get the "correct" number).

test1:
	+./jobclient echo hello

test2:
	@echo The 3 second sleep should not be able to start until hello 1 second is finished.
	+( \
	 ./jobclient ./slowecho 1 hello 1 second & \
	 ./jobclient ./slowecho 2 hello 2 second & \
	 ./jobclient ./slowecho 3 hello 3 second & \
	 wait; \
	) | ts

test3:
	@echo With jobforce, all three jobs should start simultaneously
	+( \
	 ./jobforce -1; \
	 ./jobclient ./slowecho 1 hello 1 second & \
	 ./jobclient ./slowecho 2 hello 2 second & \
	 ./jobclient ./slowecho 3 hello 3 second & \
	 wait; \
	 ./jobforce 1; \
	) | ts

test4:
	@echo "This should produce a warning about too few job tokens being available, since we've used 2 and released no jobs."
	+./jobforce 2

test5:
	@echo "This should produce no warning since the calls are balanced"
	+./jobforce 2
	+./jobforce -2

test6:
	@echo "This should produce no warning since the calls are balanced (despite being inverted!)"
	+./jobforce -2
	+./jobforce 2

test7:
	@echo This should be able to force additional jobs to be available
	+./jobforce -2
	+./jobforce 4
	+./jobforce -2

test8:
	# TODO have a test with jobforce > 2 that never finishes and check for a timeout

# It's fine to use jobforce as in test3 - releasing the implicit job for the
# duration of the process, but it's more correct to do it only while waiting.
# Some CPU will be used in the main process to set things up, so use the
# implicit job for that, then make sure we don't use up a job while we're
# blocked in wait.
test9:
	@echo "All three jobs should start simultaneously (more or less), with the last one starting after the jobforce call"
	+(\
	 ./jobclient ./slowecho 1 hello 1 second & \
	 ./jobclient ./slowecho 2 hello 2 second & \
	 ./jobclient ./slowecho 3 hello 3 second & \
	 echo "launched, now waiting..."; \
	 ./jobforce -1; \
	 wait; \
	 echo "done."; \
	 ./jobforce 1; \
	) | ts

test10:
	+test `./jobcount` -eq 2
	@echo "PASS - job count was 2"
