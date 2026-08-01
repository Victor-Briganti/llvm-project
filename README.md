# OpenMP `approx` Extension

This repository contains the reference implementation of the **OpenMP Approx construct** proposed in the paper **"Multithread Approximation: An OpenMP Construct"**.

The project extends the LLVM/Clang compiler and the OpenMP runtime to support approximation-aware parallel execution through a new OpenMP construct that enables programmers to annotate regions of code that can tolerate controlled accuracy loss in exchange for improved performance.

## Features

The current implementation supports multiple approximation techniques, including:

- Loop Perforation (`perfo`)
- Memoization (`memo`)
- Floating-point Relaxation (`fastmath`)

Each technique is implemented as an OpenMP clause and can be applied to appropriate parallel regions.

**OBS.:** Currently only one approximation technique can be active within a single `approx` region.

## Building

To build the project use the following command:

```bash 
cmake --preset omp-approx -S llvm
cmake --build build
```

## Examples

For code examples of how to use the annotations, please refer to the code on the repository: https://github.com/Victor-Briganti/approx-benchmark.

## Citation

If you use this software in academic work, please cite the corresponding paper.

```bibtex
@article{https://doi.org/10.1002/cpe.70584,
  author = {Oliveira, João Briganti and Aparecido Gonçalves, Rogério and Fabrício Filho, João},
  title = {Multithread Approximation: An OpenMP Constructor},
  journal = {Concurrency and Computation: Practice and Experience},
  volume = {38},
  number = {4},
  pages = {e70584},
  keywords = {approximation, energy efficiency, floating-point relaxation, high-performance computing, loop perforation, memoization, OpenMP, parallel context, task dropping},
  doi = {https://doi.org/10.1002/cpe.70584},
  url = {https://onlinelibrary.wiley.com/doi/abs/10.1002/cpe.70584},
  year = {2026}
}
```

