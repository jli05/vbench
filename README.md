# Benchmarking of CPU vector extension and GPU throughput
## Introduction
Traditionally-designed CPU carries out one arithmetic or logical operation per clock cycle. Techniques were invented to marginally accelerate the execution such as speculative [branch prediction](https://en.wikipedia.org/wiki/Branch_predictor), while introducing security risk such as [Spectre Attack](https://en.wikipedia.org/wiki/Spectre_(security_vulnerability)).

In contrast, CPUs with vector extension or GPUs can carry out multiple arithmetic operations per clock cycle, increasing the compute throughput by orders. The CPU state would be more deterministic, increasing introspectability and security.

A GPU is traditionally designed with many cores, which can each be programmed separately. The recent Nvidia [Tensor Cores](https://www.nvidia.com/en-us/data-center/tensor-cores/) are specialised to accelerate [matrix-matrix multiplication](https://developer.nvidia.com/blog/programming-tensor-cores-cuda-9/). For example, for 32-bit multiplication, the Tensor Core can first cast each 32-bit operand to 16-bit, do 16x16-bit multiplication to still obtain a 32-bit result. The throughput would double compared to 32x32-bit multiplication.

## Compute-bound vs Memory-bound
If after loading <math><mi>n</mi></math> elements of a vector, the number of arithmetic operations will be the same, the program is [memory-bound](https://en.wikipedia.org/wiki/Memory-bound_function). But if the number of arithmetic operations will be higher by some orders, the program is [compute-bound](https://en.wikipedia.org/wiki/CPU-bound).

Generally vector extensions can do the element-wise arithmetic operations in one clock cycle. We thus benchmark other aspects. The first four programs are memory-bound. The last is compute-bound. 

| Program  | Description |
| -------- | ------------ |
| load     | load the elements of a vector   |
| load_reduce_inter | load then sum reduction: create an all-zero `vsum` vector, first loop through and add each sub-vector element-wise to `vsum`, lastly sum reduce `vsum` to a scalar value |
| load_reduce_intra | load then sum reduction: create a zero `sum`, loop through and sum reduce each sub-vector, add the scalar result to `sum` |
| load_store | load and copy out the elements of a vector |
| gemm      | matrix-matrix multiplication |

To make the programs, for example:

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

On an [AMD Ryzen 5 AI CPU package](https://www.amd.com/en/products/processors/laptop/ryzen/ai-300-series/amd-ryzen-ai-5-340.html) with six physical cores, the base clock frequency is 2 GHz for all six, while three can ramp up to 4.8 GHz, three to 3.4 GHz.

On this CPU package, when we allocate more CPUs to `gemm`, each CPU tends to run at lower clock frequency and consume less power, and vice versa when we allocate fewer CPUs. Across the experiments, we can collect samples of observed (frequency, watt) of CPU cores, plotted as below,

![frequency-watt scatter plot](/assets/power.png)

Up to 4 GHz, the watt [scales between linearly and quadratically](/assets/power_log.png) with the clock frequency, and sheerly faster beyond 4 GHz.

Below is measured [performance per watt](https://www.brief-ds.com/2026/08/10/perf-per-watt.html) for two cases where we vary the number of running cores for `gemm`,

| Cores  |  Average MHz | GFLOP/s   | Watt |  32x32-bit GFLOP/s/watt |
| ------- | ------------ | ------------------ | ------------ | ----------------------- |
| Six  |  3400 |  501  |  15 |   33 |
| One only |  4800  |  144  |  18 |  8 |

The figures were with respect to all allocated running cores in each case.

## Notes
### Basic Linear Algebra Subprograms (BLAS)
A [Basic Linear Algebra Subgrams](https://en.wikipedia.org/wiki/Basic_Linear_Algebra_Subprograms) library is necessary for making `gemm`.

Implementations include [OpenBLAS](https://github.com/OpenMathLib/OpenBLAS) that works across architectures, or vendor-supplied ones that go further in optimisation given the characteristics of the vendor-specific chip.

The default OpenBLAS libraries supplied by Linux package managers usually do not benefit from the vector extensions. Compile from the source code:

```sh
git clone https://github.com/OpenMathLib/OpenBLAS
cd OpenBLAS
# simply issue make
# it will be optimally configured for hardware features
make
sudo make install PREFIX=/opt/OpenBLAS

# on ARM Aarch64 with Scalable Vector Extension
gcc -O -march=armv8-a+sve gemm.c timer.c -L/opt/OpenBLAS/lib -lopenblas -lm -o gemm
LD_LIBRARY_PATH=/opt/OpenBLAS/lib ./gemm

# on x86-64 with Advanced Vector Extension
gcc -O -mavx512f gemm.c timer.c -L/opt/OpenBLAS/lib -lopenblas -lm -o gemm
LD_LIBRARY_PATH=/opt/OpenBLAS/lib ./gemm
```

Using vendor-supplied BLAS library may secure higher performance, while OpenBLAS has the advantage that it is more widely used and tested.

For example, on AMD, download the AOCL BLAS from the AMD Optimizing CPU Libraries (AOCL) page [https://www.amd.com/en/developer/aocl.html](https://www.amd.com/en/developer/aocl.html). Unpack it to for example `/opt/amd-blis`, then make and run `gemm`.

```sh
# on AMD with Advanced Vector Extension
gcc -O -mavx512f gemm.c timer.c -L/opt/amd-blis/lib/LP64 -lblis-mt -lm -o gemm
LD_LIBRARY_PATH=/opt/amd-blis/lib/LP64 ./gemm
```

### sum reduce signed integers on ARM Aarch64
To sum reduce a vector of signed integers lower than 64-bit, directly do `saddv`:

```asm
    saddv d11, p7, z30.s
    fmov  x11, d11
```

To sum reduce a vector of signed 64-bit integers, first convert to 64-bit floating-point numbers, then convert back after the reduction.

```asm
    ptrue   p7.d
    scvtf   z30.d, p7/m, z30.d
    faddv   d11, p7, z30.d
    fcvtms  x10, d11
```

Other than the pecularity, note from the experiment results that reduction inter-vector (via element-wise operation) can be faster than intra-vector.
