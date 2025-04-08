Liam Perez lperez19
Immanuel Gebreyesus igebrey1

Analysis of Experiments

Raw output of experiments disclosed below.

While there was some unexpected output (threshold 32768 performed unexpectedly well),
the real time seems to decrease as the size of the partition threshold increases. 
Accounting for the possibility of variances on the client's end outside of the program's control,
it appears that our code is exhibiting parallel behaviors due to the positive effects
a higher parallel threshold has on speed.

An explanation for why this would occur is that the OS kernel is able to be more "productive"
in terms of the sorting algorithm when the partition increases. In the case of the normal quicksort,
the entire computation is one process; this means when the OS kernel timer shifts focus away from this one 
process, there is no progress being made on the operation. However, with the parallel implementation 
and a larger threshold, each individual child process is sorting its own small segment of the array. 
This means, as long as any given CPU core is focusing on any child process, progress is being made.
More practically, it means that multiple CPU cores will be focusing on different child processes, 
allowing many arbitrary slices of the array to be sorted at once for faster overall performance.

Of course, parallel sorting nearly halved the time required across the board, which makes sense considering 
multiple processes can run at the same time without waiting on the processes until the result gets back to 
the parent function. All in all, the parallel processing greatly improves the performance of the sort and should 
be used as much as possible for repeptitive tasks in programming.

## Output of Experiments (SERIAL):

Test run with threshold 2097152

real    0m0.699s
user    0m0.672s
sys     0m0.022s
Test run with threshold 1048576

real    0m0.873s
user    0m0.810s
sys     0m0.054s
Test run with threshold 524288

real    0m0.883s
user    0m0.789s
sys     0m0.081s
Test run with threshold 262144

real    0m0.903s
user    0m0.807s
sys     0m0.084s
Test run with threshold 131072

real    0m0.943s
user    0m0.849s
sys     0m0.082s
Test run with threshold 65536

real    0m0.972s
user    0m0.851s
sys     0m0.110s
Test run with threshold 32768

real    0m0.909s
user    0m0.747s
sys     0m0.153s
Test run with threshold 16384

real    0m1.074s
user    0m0.846s
sys     0m0.219s

## Output of Experiments (PARALLEL):

Test run with threshold 2097152

real    0m0.375s
user    0m0.361s
sys     0m0.013s
Test run with threshold 1048576

real    0m0.218s
user    0m0.374s
sys     0m0.032s
Test run with threshold 524288

real    0m0.166s
user    0m0.434s
sys     0m0.036s
Test run with threshold 262144

real    0m0.130s
user    0m0.447s
sys     0m0.040s
Test run with threshold 131072

real    0m0.122s
user    0m0.450s
sys     0m0.051s
Test run with threshold 65536

real    0m0.113s
user    0m0.472s
sys     0m0.073s
Test run with threshold 32768

real    0m0.278s
user    0m0.503s
sys     0m0.100s
Test run with threshold 16384

real    0m0.125s
user    0m0.520s
sys     0m0.164s