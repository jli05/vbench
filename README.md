# Benchmarking of CPU vector extension or GPU compute throughput
## Introduction
Traditionally-designed CPU carries out arithmetic or logical compute one per clock cycle. Afterwards techniques such as speculative [branch prediction](https://en.wikipedia.org/wiki/Branch_predictor) were invented, which would accelerate execution only marginally, whilst introducing security risk such as [Spectre Attack](https://en.wikipedia.org/wiki/Spectre_(security_vulnerability)).

The CPU vector extensions or GPUs can carry out multiple arithmetic operations per clock cycle, increasing the compute throughput by orders. The CPU state would be more deterministic executing vector instructions, largely increasing introspectability and security.

A GPU is traditionally designed with many cores, which can each be separately programmed, for graphics or scientific processing. The recent Nvidia [Tensor Cores](https://www.nvidia.com/en-us/data-center/tensor-cores/) are specialised for [matrix-matrix multiplication](https://developer.nvidia.com/blog/programming-tensor-cores-cuda-9/). The precision of the operands can be reduced. For example, for 32-bit multiplication, they first cast each 32-bit operand to 16-bit, multiply them to obtain a 32-bit result. The throughput would double compared to multiplication of two 32-bit operands.

## Compute-bound vs Memory-bound
If after loading <math><mi>n</mi></math>

| load     | read the elements of a vector   |
| load_store | read and copy out the elements of a vector |
| load_reduce_inter | sum reduction, first add each sub-vector to a vector, lastly sum reduce that vector to a scalar value |
| load_reduce_intra | sum reduction,

## Results


## Notes

```sh
export PATH=$PATH:/usr/local/cuda/bin
nvcc -O1 gemm.cuda.c -lcublas
./a.out
```


