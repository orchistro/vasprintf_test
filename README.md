# Performance test for vsnprintf vs. vasprintf

To make a function that returns an instance of `std::string` with printf-like interface,
we need to use one of `printf` family inside the function.
We have lots of members in the `printf` family: `vsnprintf` and `vasprintf` are among them.

If we were to use `vsnprintf`, we first need to calculate the length of resulting string.
To do that, we have to call `vsnprintf` with `nullptr':

```c++
len = vsnprintf(nullptr, 0, format, ap);
```

and then, we can allocate the memory needed and proceed with `vsnprintf`;

```c++
buffer = (char*)malloc(len + 1);
vsnprintf(buffer, len+1, format, ap);
```

But, we can remove the first call to `vsnprintf` to calculate the size of buffer needed if we use `vasprintf`.
A good news. But, how about performance? Which one would be faster? 
The one with two call to `vasprintf`, or the one with `vasprintf`?

https://github.com/orchistro/vasprintf_test/blob/master/foo.cpp

This is the test code to find out which one is faster.

The result seems to depend on the libc versions.
On macos, they perform very close to each other, actually `vsnprintf` is slightly faster.

```bash
user@YoungHwiMBP13.local:~/work/orchistro/vasprintf_test$ ./foo 3000000 5 64 2> /dev/null
Generating 3,000,000 strings of length 5 ~ 64
Testing with method vasprintf    --> Took 10,496,436us (10,496ms)
Testing with method vsnprintf    --> Took 10,854,069us (10,854ms)
Testing with method vasprintf    --> Took 10,501,619us (10,501ms)
Testing with method vsnprintf    --> Took 10,849,487us (10,849ms)
Testing with method vasprintf    --> Took 10,932,630us (10,932ms)
Testing with method vsnprintf    --> Took 11,099,152us (11,099ms)
Testing with method vasprintf    --> Took 10,724,805us (10,724ms)
Testing with method vsnprintf    --> Took 11,144,898us (11,144ms)

```

And the disparity gets bigger as the length of the longest string gets bigger.

```bash
user@YoungHwiMBP13.local:~/work/orchistro/vasprintf_test$ ./foo 3000000 5 512 2> /dev/null
Generating 3,000,000 strings of length 5 ~ 512
Testing with method vasprintf    --> Took 11,184,391us (11,184ms)
Testing with method vsnprintf    --> Took 11,550,055us (11,550ms)
Testing with method vasprintf    --> Took 11,216,067us (11,216ms)
Testing with method vsnprintf    --> Took 11,788,046us (11,788ms)
Testing with method vasprintf    --> Took 11,171,376us (11,171ms)
Testing with method vsnprintf    --> Took 12,331,870us (12,331ms)
Testing with method vasprintf    --> Took 11,899,398us (11,899ms)
Testing with method vsnprintf    --> Took 12,113,261us (12,113ms)
```

My macbook pro has 64bytes cache lines:

```bash
user@YoungHwiMBP13.local:~/work/orchistro/vasprintf_test$ ./foo cpuinfo
Cache ID 0:
- Level: 1
- Type: Data Cache
- Sets: 64
- System Coherency Line Size: 64 bytes
- Physical Line partitions: 1
...
```

But on a linux machine, it is very different story

For small size (less than cache line size) strings, practically no difference. Sometimes `vsnprintf` even beats `vasprintf`.

```bash
irteam@cmiddev02.nm:~/shawn/test/vasprintf_test$ ./foo 3000000 5 64 2> /dev/null
Generating 3,000,000 strings of length 5 ~ 64
Testing with method vasprintf    --> Took 4,287,323us (4,287ms)
Testing with method vsnprintf    --> Took 4,212,696us (4,212ms)
Testing with method vasprintf    --> Took 4,178,516us (4,178ms)
Testing with method vsnprintf    --> Took 4,140,725us (4,140ms)
Testing with method vasprintf    --> Took 4,217,055us (4,217ms)
Testing with method vsnprintf    --> Took 4,199,996us (4,199ms)
Testing with method vasprintf    --> Took 3,891,535us (3,891ms)
Testing with method vsnprintf    --> Took 4,011,994us (4,011ms)
```

However, when the data gets bigger than the cache line size, `vasprintf` is even faster than `vsnprintf`.

```bash
cmiddev02.nm:~/shawn/test/vasprintf_test$ ./foo 3000000 5 512 2> /dev/null
Generating 3,000,000 strings of length 5 ~ 512
Testing with method vasprintf    --> Took 5,181,194us (5,181ms)
Testing with method vsnprintf    --> Took 7,904,855us (7,904ms)
Testing with method vasprintf    --> Took 5,280,120us (5,280ms)
Testing with method vsnprintf    --> Took 7,975,132us (7,975ms)
Testing with method vasprintf    --> Took 5,325,634us (5,325ms)
Testing with method vsnprintf    --> Took 7,897,318us (7,897ms)
Testing with method vasprintf    --> Took 5,257,921us (5,257ms)
Testing with method vsnprintf    --> Took 7,759,422us (7,759ms)
```

Linux machine is on Inter Xeon:
```bash
irteam@cmiddev02.nm:~/shawn/test/vasprintf_test$ uname -a
Linux cmiddev02.nm 2.6.32-642.13.1.el6.x86_64 #1 SMP Wed Jan 11 20:56:24 UTC 2017 x86_64 x86_64 x86_64 GNU/Linux
irteam@cmiddev02.nm:~/shawn/test/vasprintf_test$ cat /proc/cpuinfo
...
processor       : 39
vendor_id       : GenuineIntel
cpu family      : 6
model           : 79
model name      : Intel(R) Xeon(R) CPU E5-2630 v4 @ 2.20GHz
stepping        : 1
microcode       : 184549405
cpu MHz         : 2194.849
cache size      : 25600 KB
physical id     : 1
siblings        : 20
core id         : 12
cpu cores       : 10
apicid          : 57
initial apicid  : 57
fpu             : yes
fpu_exception   : yes
cpuid level     : 20
wp              : yes
flags           : fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush dts acpi mmx fxsr sse sse2 ss ht tm pbe syscall nx pdpe1gb rdtscp lm constant_tsc arch_perfmon pebs bts rep_good xtopology nonstop_tsc aperfmperf pni pclmulqdq dtes64 ds_cpl vmx smx est tm2 ssse3 fma cx16 xtpr pdcm pcid dca sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand lahf_lm abm 3dnowprefetch arat epb xsaveopt pln pts dtherm tpr_shadow vnmi flexpriority ept vpid fsgsbase bmi1 hle avx2 smep bmi2 erms invpcid rtm cqm rdseed adx cqm_llc cqm_occup_llc
bogomips        : 4389.51
clflush size    : 64
cache_alignment : 64
address sizes   : 46 bits physical, 48 bits virtual
power management:
```

