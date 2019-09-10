# GNU Make jobserver and jobclient #

(and some related utilities)

Primarily, this allows shell scripts and other software to submit jobs to a
running Make job server, waiting for free slots (usually corresponding to CPUs)
to become available before they start.

To submit a job, instead of running your command directly, prepend `jobclient`
to it and run it in the background. The `jobclient` utility will contact the
job server, wait for a free job slot (without using CPU), then launch the
command given as its arguments.


## Utilities ##

### jobclient ###

From a shell script, a convenient way to use this would be to first submit all
jobs, then use `wait` without arguments to wait for all children at once, for
example:

    $ for f in *; do jobclient process_file $f & done; wait

Note that every job has a running process even if it is blocked - if the number
of jobs is truly large, that might cause trouble in itself. In the future there
might be a more shell-friendly way to move the job server communication "up one
step" to avoid starting a process when there are no slots available.

### jobserver ###

This takes the place of the top-level Make process and provides a jobserver for
its child process. If the child is itself Make, the jobserver will communicate
with it using the `MAKEFLAGS` environment variable.

`jobserver` can also be used to "hide" an ambient jobserver from a child process.

### jobforce ###

For "advanced" usage (or just playing around with confusing Make), there's also
a `jobforce` utility that can artificially consume job slots or create/release
job slots.

`jobforce` takes a single integer argument that's the number of job slots to
consume, blocking until the number of job slots has been acquired. A negative
number releases/creates job slots.

Note that Make wants the job slot count to be balanced on exit, so `jobforce`
calls should be balanced such that the the sum of the arguments comes out as 0.

### jobcount ###

Just for curiosity, statistics, or questionable usability, there's also a job
count utility that prints the number of free job slots at the time of the call.

It works by trying to acquire up to 4096 job slots without blocking, then
releasing them after printing the number of job slots acquired.

## Using job clients from Make ##

To use a jobclient-enhanced utility from Make, make sure to prepend the command
line with a `+` sign. This sets `MAKEFLAGS` in the subprocess and exposes the
jobserver file descriptors.

### Dealing with GNU Make's implicit job ###

The main process launched by GNU Make has one job slot implicitly assigned to
itself - this was probably quite convenient for a client like Make that can
simply launch "one more" process than the job server allows.


When used in the simple launch-then-wait manner from a shell script though, the
implicit job remains occupied while the main script is blocked on wait. This
means that the effective parallelism is one lower than the user requested.

A hacky workaround is to use the jobforce command to increase the number of
job slots (by releasing one that was never actually acquired), then wait, then
take back the extra freed-up job. For example:

    $ [launch many jobs]; jobforce -1; wait; jobforce 1


You can also work around it by running a single "last" piece of work (if there
is one) without wrapping it in `jobclient`.

## License ##

All the code here is MIT licensed.
