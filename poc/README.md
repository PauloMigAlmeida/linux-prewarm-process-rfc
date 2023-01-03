poc
===

## Pre-warm processes at the beginning of the script

We mimic the execve syscall in userspace and programs are
loaded in parallel using pthreads. This was the implementation
up to commit f5302eb450e458011e56470883932b0c2d2cf56b but I had
to abandon this approach as the time spent creating and executing
those pre-warming threads was impacting the main program execution
time.

It yielded some performance gains but the number of processes to be
launched in a bash script would have to be around 2500 before we
could reach the inflection point. For instance:

```
Running in pre-warm mode with 5000 processes took 0.016577848 seconds
```
```
Running in normal mode with 5000 processes took 0.004976643 seconds
```

Those are good numbers (almost 3x faster) but expecting a script to
have 5000 programs to be executed is a bit far-fetched so I abandoned
this approch for good.

## on-demand loading processes (and keep them warm after that)

In this approach we don't use threads at all and all that we do is
to cache processes as they are launched so if they happen to be used
more than once then it won't have to be loaded from disk.

This yielded much better results as you can start seeing some 
performance boost with as little as 2 repeated programs in a given
bash script. (essentially, the more programs that repeat, the better
the results will be... which I believe is fine given that GNU coreutils
tend to repeat a lot in scripts after all...)

**Normal Mode**

```bash
echo 0 > /proc/sys/kernel/nmi_watchdog
echo 3 > /proc/sys/vm/drop_caches
perf stat -r 100 --sync -B -- ./build/main/main 0 2 build/cli_utilities
echo 1 > /proc/sys/kernel/nmi_watchdog

 Performance counter stats for './build/main/main 0 2 build/cli_utilities' (100 runs):

              0.54 msec task-clock                       #    0.601 CPUs utilized            ( +-  2.07% )
                 1      context-switches                 #    1.949 K/sec                    ( +-  4.02% )
                 0      cpu-migrations                   #    0.000 /sec                   
                60      page-faults                      #  116.954 K/sec                  
           904,793      cycles                           #    1.764 GHz                      ( +-  2.20% )
            61,013      stalled-cycles-frontend          #    6.81% frontend cycles idle     ( +-  2.87% )
           100,205      stalled-cycles-backend           #   11.19% backend cycles idle      ( +-  2.06% )
           951,214      instructions                     #    1.06  insn per cycle         
                                                  #    0.11  stalled cycles per insn  ( +-  2.73% )
           236,286      branches                         #  460.576 M/sec                    ( +-  2.64% )
             6,659      branch-misses                    #    2.73% of all branches          ( +-  1.91% )

         0.0009070 +- 0.0000239 seconds time elapsed  ( +-  2.63% )
```

**On-demand loading mode**

```bash
echo 0 > /proc/sys/kernel/nmi_watchdog
echo 3 > /proc/sys/vm/drop_caches
perf stat -r 100 --sync -B -- ./build/main/main 1 2 build/cli_utilities
echo 1 > /proc/sys/kernel/nmi_watchdog


 Performance counter stats for './build/main/main 1 2 build/cli_utilities' (100 runs):

              0.46 msec task-clock                       #    0.527 CPUs utilized            ( +-  2.64% )
                 0      context-switches                 #    0.000 /sec                   
                 0      cpu-migrations                   #    0.000 /sec                   
                60      page-faults                      #  120.036 K/sec                  
           810,770      cycles                           #    1.622 GHz                      ( +-  2.66% )
            49,035      stalled-cycles-frontend          #    5.62% frontend cycles idle     ( +-  3.67% )
            94,107      stalled-cycles-backend           #   10.79% backend cycles idle      ( +-  3.22% )
           936,688      instructions                     #    1.07  insn per cycle         
                                                  #    0.11  stalled cycles per insn  ( +-  2.88% )
           232,483      branches                         #  465.105 M/sec                    ( +-  2.78% )
             3,965      branch-misses                    #    1.66% of all branches          ( +-  3.09% )

         0.0008805 +- 0.0000244 seconds time elapsed  ( +-  2.77% )
```


## Notes regarding reproducibility

In order to get consistent results the following script was executed
to avoid getting tricked by some of the HW and OS features.

```bash
 Disable turboboost for AMD (Intel's is "echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo")
echo 0 > /sys/devices/system/cpu/cpufreq/boost

# Let kernel know that we don't want any of those power save modes
for i in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
do
  echo performance > $i
done

# Disable HyperThreading
echo off > /sys/devices/system/cpu/smt/control

# Disable Address space randomisation
echo 0 | sudo tee /proc/sys/kernel/randomize_va_space
```

## How to build this PoC

You have to have `make`, `gcc` and `nasm` installed.

```bash
cd <git_repo>/poc
make
```

To run the main program, just refer to the reproducibility
notes in this readme + the performance results. They contain
the commands executed to obtain the aforementioned results.

