# Parallel Programming Activities

Cristopher Ochoa Villanueva <br>

## Installation

### C++ version

```
g++.exe (MinGW-W64 x86_64-msvcrt-posix-seh, built by Brecht Sanders, r1) 15.2.0
```

## Compilation

### 1. C++ Language Standard - Libraries

```
g++ -c main.cpp -o main.o
g++ -c my_math.cpp -o my_math.o
g++ main.o my_math.o -o programa.exe
```

### 2. Create PPM Image

```
g++ generador.cpp -o generador.exe
g++ filtro.cpp -o filtro.exe
```

### 3. JPG Image Processing with the STB Library (Sequential and Multithreaded)

```
g++ main.cpp -o tarea -O3
```

### PROJECT: PAVIC LAB 2025

```
g++ main.cpp -o app_pro -O2 -lgdi32 -lcomdlg32 -fopenmp
```
