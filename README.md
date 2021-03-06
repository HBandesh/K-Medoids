# K-Medoids

## Description
KMedoids library written in C++17 using the Partition Around Medoids<sup>[1]</sup> (PAM) BUILD and SWAP algorithms, as well as the CLARA<sup>[2]</sup> approximation algorithm. The PAM<sup>[1]</sup> algorithms have been implemented for both serial and multi-threaded execution, whereas the CLARA algorithm has been implemented for serial, multi-threaded, distributed, and hybrid (multi-threaded and distributed) execution. Multi-threaded and distributed execution were implemented using the OpenMP and OpenMPI libraries. Both the parallelism and the distance function can be changed statically at compile time. Benchmark runs on a centOS cluster using a dataset consisting of 10,000 datapoints each with dimensionality of 10, organized into 10 clusters, where each cluster was generated from a guassian distribution with standard deviation of 6 in a (-100, 100) box can be seen below. The times reported are the averages over 10 independent runs. In addition, the CLARA algorithm used 10 repeats for each independent run. The scripts used to generate the test data and plot the results can be found in the __results__ directory.

<div style="text-align:center">
<img src="results/plots/omp_pam.png" width="300"/>
<img src="results/plots/omp_clara.png" width="300"/>
</div>
<div style="text-align:center">
<img src="results/plots/mpi_clara.png" width="300"/>
<img src="results/plots/hybrid_clara.png" width="300"/>
</div>

## Dependencies
- C++17
- Boost 1.72.0
- OpenMPI 4.0.2
- OpenMP compatible C++ compiler
- [Matrix](https://github.com/e-dang/Matrix)


## Compiling
The top level __CMakeLists.txt__ is set to statically compile the hpkmedoids library, as well as binaries for every combination of parallelism, PAM, and CLARA algorithms using the __main.cpp__ src program.

Tested on MacOS Mojave 10.14.6 and CentOS with compilers:
- Clang 9.0.1
- GCC 7.2.0

In the top level directory of the project, run the following commands:
```
mkdir build
cd build
cmake ..
make
```

## Usage
This library may be imported into an existing C++ project in which case you need to compile and link the hpkmedoids library to your project, then add the include statement to the top level __kmedoids.hpp__ header file to start using it.
```
#include <hpkmedoids/kmedoids.hpp>
```

There is also a __main.cpp__ src file included which can be edited to specify a filepath to a data file, the dimensions of the data, the hyperparameters, the parallelism, and the algorithms to use for clustering. Compiling and running this program will produce result files containing the centroids, assignments, and error in the same directory as the data file.

## Citations
1. Kaufmann, Leonard & Rousseeuw, Peter. (1987). [Clustering by Means of Medoids](https://www.researchgate.net/publication/243777819_Clustering_by_Means_of_Medoids#:~:text=As%20a%20kind%20of%20basic,marginal%20space%20between%20the%20points.). Data Analysis based on the L1-Norm and Related Methods. 405-416.
2. Kaufman, L., Rousseeuw, P.J.: [Clustering large data sets](https://www.sciencedirect.com/science/article/pii/B978044487877950039X?via%3Dihub). In: Pattern Recognition in Practice. Elsevier (1986). https://doi.org/10.1016/b978-0-444-87877-9.50039-x

## Authors
Eric Dang