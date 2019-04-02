# Multithreaded calculation in C++ (forks and p_threads)


### Compile
```sh
$ g++ -std=c++11 -g main.cpp -o main.out -pthread
```

### Launch
```sh
$ ./main.out MATRIX_N MODE INSTANCES_NUMBER
```
#### Params

| Param | Defenition |
| ------ | ----------|
| MATRIX_N | number of elements in matrix N x N  |
| MODE | 1 - for forks; 2 - for threads; any other for serial calculation |
| INSTANCES_NUMBER  | depends on MODE, how meny instances of forks/threads to create |

#### Calculations
> This program finds multiplication of avarage values of indexes of all zero valued elements in each line and column of the given matrix

