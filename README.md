# Benchmarking of CPU vector extension or GPU compute throughput
## Introduction
Traditionally-designed CPU carries out arithmetic or logical compute one per clock cycle. Afterwards techniques such as speculative [branch prediction](https://en.wikipedia.org/wiki/Branch_predictor) were invented, which would accelerate execution only marginally, whilst introducing security risk such as [Spectre Attack](https://en.wikipedia.org/wiki/Spectre_(security_vulnerability)).

The CPU vector extensions or GPUs can carry out multiple arithmetic operations per clock cycle, increasing the compute throughput by orders. The CPU state would be more deterministic executing vector instructions, largely increasing introspectability and security.

A GPU is traditionally designed with many cores, which can each be separately programmed, for graphics or scientific processing. The recent Nvidia [Tensor Cores](https://www.nvidia.com/en-us/data-center/tensor-cores/) are specialised for [matrix-matrix multiplication](https://developer.nvidia.com/blog/programming-tensor-cores-cuda-9/). The precision of the operands can be reduced. For example, for 32-bit multiplication, one may first cast each 32-bit operand to 16-bit, multiply them to obtain a 32-bit result. The throughput would double compared to multiplication of two 32-bit operands.

## Compute-bound vs Memory-bound
If after loading <math><mi>n</mi></math> elements of a vector, the number of arithmetic operations will be the same, the program is [memory-bound](https://en.wikipedia.org/wiki/Memory-bound_function). But if the number of arithmetic operations will be higher by some orders, the program is [compute-bound](https://en.wikipedia.org/wiki/CPU-bound).

The first four benchmarking programs are memory-bound. The last is compute-bound.

| Program  | Description |
| -------- | ------------ |
| load     | read the elements of a vector   |
| load_reduce_inter | sum reduction: create an all-zero `vsum` vector, first loop through and add each sub-vector to `vsum`, lastly sum reduce `vsum` to a scalar value |
| load_reduce_intra | sum reduction: create a zero `sum`, loop through and sum reduce each sub-vector, add to `sum` |
| load_store | read and copy out the elements of a vector |
| gemm      | matrix-matrix multiplication |

To compile them, for example:

```sh
# on ARM Aarch64 with Scalable Vector Extension
gcc -O -march=armv8-a+sve load_reduce_inter.sve.c load_reduce_inter.sve.s timer.c -o load_reduce_inter.sve
gcc -O -march=armv8-a+sve cpufreq.sve.c cpufreq.sve.s -o cpufreq.sve
gcc -O -march=armv8-a+sve gemm.c timer.c -L<blas> -l<blas> -lm -o gemm
```

```sh
# on x86-64 with Advanced Vector Extension
gcc -O -mavx512f load_store.avx.c timer.c -o load_store.avx
gcc -O -mavx512f gemm.c timer.c -L<blas> -l<blas> -lm -o gemm
```

```sh
# on Nvidia GPU
export PATH=$PATH:/usr/local/cuda/bin
nvcc -O1 gemm.cuda.c -lcublas -o gemm.cuda
```

The vector elements are 32-bit. To change to 64-bit elements, in the C file, change `elem_type` for example

```c
    typedef long elem_type;
```

and make other necessary change.

## Results
Below we highlight some results.

### memory-bound tasks
[Graviton3](https://en.wikipedia.org/wiki/AWS_Graviton) is an ARM processor designed by Amazon Web Services, with 512-bit vector processing width. On one vector of 512 bits,

| Program | CPU clock cycles  |
| ------- | ----------------- |
| load    |  4.8  |
| load_reduce_inter | 5 |
| load_reduce_intra | 6.5 |
| load_store | 8.2  |

So the loading or writing of data takes the longest time. Reduction intra-vector takes slightly longer than inter-vector in this instance.

### compute-bound task
On x86-64, use command `taskset` to specify the CPUs to execute a program on,

```sh
# to run on CPUs with id 0, 2 and 4
taskset -c 0,2,4 ./gemm
```

In another shell, visualise the clock frequency and power consumption,

```sh
sudo turbostat --show CPU,frequency,CorWatt,PkgWatt
```

On an AMD CPU package, when we allocate more CPUs to `gemm`, each CPU tends to run at lower clock frequency and consume less power, and vice versa when we allocate fewer CPUs.

Across settings, we can collect samples of the data pair (frequency, watt) of CPU cores, plotted as below,

![frequency-watt scatter plot](/assets/power.png)

Actually, up to 4 GHz, the watt [scales quadratically](/assets/power_log.png) with the clock frequency, and much faster once beyond 4 GHz.



## Notes
To sum reduce a vector of 64-bit integers, on ARM Aarch64 SVE, first convert to 64-bit floating-point numbers, then convert back after the reduction.

```asm
    ptrue   p7.d
    scvtf   z30.d, p7/m, z30.d
    faddv   d11, p7, z30.d
    fcvtms  x10, d11
```


