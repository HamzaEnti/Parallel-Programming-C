# High-Performance Computing Portfolio: Parallel Algorithms in C

This repository serves as a comprehensive portfolio demonstrating advanced concepts in Parallel Computing using the C programming language. It focuses on the two primary paradigms of high-performance computing (HPC): Distributed Memory utilizing the Message Passing Interface (MPI) and Shared Memory utilizing OpenMP.

The projects included in this repository explore critical aspects of parallel software design, including domain decomposition, load balancing, synchronization overhead reduction, and race condition management.

## Project Overview

The repository is divided into two distinct modules, each addressing a specific computational problem solved through parallelization strategies.

### 1. Distributed Prime Number Counter (MPI)
**Directory:** mpi_prime_numbers/

This module solves a data-parallel problem: identifying and counting prime numbers within a massive dataset. In a distributed memory environment, processors cannot access a shared address space. Therefore, this implementation explicitly manages memory and communication between nodes.

**Key Technical Features:**
* **Domain Decomposition:** The root process generates the initial dataset (array of integers). This dataset is logically split into chunks.
* **MPI_Scatter:** Instead of sending the entire array to every node (which would exhaust memory), the program uses `MPI_Scatter` to distribute only the specific workload chunk to each worker process.
* **Local Computation:** Each process executes a primality test (Trial Division algorithm) on its local buffer independently.
* **MPI_Reduce:** The partial results (local counts) are aggregated back to the root process using a reduction operation with the `MPI_SUM` operator.
* **Token Ring Topology:** After the primary computation, the program establishes a logical ring topology where a "token" is passed sequentially from process `i` to `i+1`. This demonstrates strict point-to-point synchronization and message ordering (`MPI_Send` and `MPI_Recv`).

### 2. SHA-256 Password Cracker (OpenMP)
**Directory:** openmp_password_crack/

This module implements a multithreaded brute-force attack to reverse a SHA-256 cryptographic hash. The goal is to find a 6-character alphanumeric password corresponding to a given hash target. This project is particularly significant as it demonstrates the evolution of code optimization.

**Implemented Versions & Optimization Analysis:**

* **Version 1 (Naive Parallelization):**
    * **Strategy:** Launches parallel threads where each thread attempts to find the password.
    * **Bottleneck:** It checks the global boolean flag `trobat` (found) in every single iteration. This causes "False Sharing" and high bus traffic because the cache line containing the flag is constantly being invalidated and updated across cores.
    * **Result:** High CPU overhead and suboptimal scaling.

* **Version 2 (Dynamic Scheduling):**
    * **Strategy:** Uses `schedule(dynamic, chunk_size)` to assign blocks of iterations to threads at runtime.
    * **Use Case:** This is typically useful when iterations take varying amounts of time. However, in this specific cryptographic scenario, hashing takes constant time, so the overhead of dynamic scheduling management may slightly degrade performance compared to static scheduling.

* **Version 3 (Optimized - Static & Local Buffering):**
    * **Strategy:** This is the most efficient implementation.
    * **Optimization 1 (Scheduling):** Uses `schedule(static)` to divide the iteration space (seeds) evenly at compile/launch time, eliminating runtime scheduling overhead.
    * **Optimization 2 (Reduced Synchronization):** Instead of checking the global `trobat` flag in every iteration, each thread maintains a `local_check_counter`. The thread only checks the global shared variable once every 65,536 iterations.
    * **Impact:** This drastically reduces atomic operations and memory bus contention, allowing the CPU to spend nearly 100% of its cycles on computing SHA-256 hashes rather than synchronization.

## Technical Requirements

To compile and run these projects, the following dependencies are required:

* **Compiler:** GCC (GNU Compiler Collection).
* **MPI Library:** OpenMPI or MPICH (provides `mpicc`, `mpirun`).
* **Cryptography Library:** OpenSSL (`libssl-dev`) for SHA-256 computation.
* **Math Library:** Standard C math library (`-lm`).

## Installation and Compilation

A centralized `Makefile` is provided to streamline the build process. It handles the linking of necessary libraries (OpenSSL, Math) and applies compiler optimizations (`-O3`).

1.  **Clone the repository:**
    (Assuming you have git installed)
    git clone https://github.com/HamzaEnti/Parallel-Programming-C.git
    cd Parallel-Programming-C

2.  **Build the solution:**
    Execute the following command in the root directory:
    make

    This will generate the following executables:
    * `mpi_prime_numbers/mpi_seq` (Sequential baseline)
    * `mpi_prime_numbers/mpi_par` (Parallel MPI)
    * `openmp_password_crack/omp_seq` (Sequential baseline)
    * `openmp_password_crack/omp_v1` (Parallel V1)
    * `openmp_password_crack/omp_v2` (Parallel V2)
    * `openmp_password_crack/omp_v3` (Parallel V3)

3.  **Clean build artifacts:**
    To remove object files and executables:
    make clean

## Usage Guide

### Running the Distributed Prime Counter (MPI)

The MPI program requires the `mpirun` wrapper to spawn processes. The program accepts one argument: the size of the array to be generated.

Syntax:
mpirun -n <number_of_processes> ./mpi_prime_numbers/mpi_par <array_size>

Example (Running on 4 cores with 10 million elements):
mpirun -n 4 ./mpi_prime_numbers/mpi_par 10000000

*Note: For the Scatter operation to work perfectly in this educational example, ensure the array_size is divisible by the number_of_processes.*

### Running the SHA-256 Cracker (OpenMP)

The OpenMP programs run directly as standard executables. They utilize all available CPU cores by default (or the number specified in the `OMP_NUM_THREADS` environment variable).

Syntax:
./openmp_password_crack/omp_v3 <max_iterations> <password_length> <target_hash>

Example Scenario:
We want to recover the password for the hash corresponding to the string "abc".
Target Hash: ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad

Command:
./openmp_password_crack/omp_v3 1000000 3 ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad

## Performance Benchmarks

Below is a qualitative analysis of the expected performance based on the implementation strategies:

1.  **MPI Scaling:** The Prime Counter shows linear speedup as the dataset size ($N$) increases. For small $N$, the communication overhead of `MPI_Scatter` may result in performance lower than the sequential version. Efficiency increases significantly when the computation time per chunk exceeds the network transmission time.

2.  **OpenMP Optimization:**
    * **V1:** Experience significant slowdowns on high-core-count machines due to atomic contention.
    * **V3:** Should demonstrate near-ideal linear speedup ($Speedup \approx Number\_of\_Cores$). The optimization of checking the stop condition only periodically (every 64k iterations) is the critical factor allowing this scalability.

---
Developed as a study on High-Performance Computing architectures and parallel programming patterns.
