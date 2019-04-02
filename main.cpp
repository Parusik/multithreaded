#include <iostream>
#include <vector>
#include <unistd.h>
#include <numeric>
#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "worker.hpp"

using namespace std;

double serialWorker(const Matrix &matrix, const int &n)
{
    const int startIndex = 0;
    const int endIndex = matrix.size();

    return worker(matrix, n, startIndex, endIndex);
}

double threadWorker(Matrix &matrix, const int &n, const int &threadsCount)
{
    // Calculate workload
    const int vectorsCount = matrix.size();
    const int rowsPerFork = vectorsCount / threadsCount;
    const int restRows = vectorsCount % threadsCount;

    int startIndex, endIndex;

    // init message queue
    queue<double> queue;
    pthread_t *threads = new pthread_t[threadsCount]();
    pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

    for (int threadIndex = 0; threadIndex < threadsCount; threadIndex++)
    {

        int startIndex = threadIndex == 0 ? 0 : rowsPerFork * threadIndex + restRows;
        int endIndex = rowsPerFork * (threadIndex + 1) + restRows;
        // worker_context context = {matrix, n, startIndex, endIndex, queue};
        worker_context *context = new worker_context{matrix, n, startIndex, endIndex, queue};

        if (pthread_create(&threads[threadIndex], NULL, workerThreadWrapper, context))
        {
            fprintf(stderr, "Error creating thread\n");
            exit(-1);
        }
    }

    // wait untill threads finish
    for (int i = 0; i < threadsCount; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // get results from message queue
    long qsize = queue.size();

    double result = 0;
    for (int i = 0; i < qsize; i++)
    {
        if (result == 0)
        {
            result = queue.front();
        }
        else
        {
            result *= queue.front();
        }
        queue.pop();
    }
    return result;
}

double forkWorker(const Matrix &matrix, const int &n, const int &forksCount)
{

    // Calculate workload
    const int vectorsCount = matrix.size();
    const int rowsPerFork = vectorsCount / forksCount;
    const int restRows = vectorsCount % forksCount;
    double result = 0;
    int pid, pipes[2];

    if (pipe(pipes) == -1)
    {
        perror("error creating pipe");
        exit(-1);
    }

    for (int forkIndex = 0; forkIndex < forksCount; forkIndex++)
    {
        switch (pid = fork())
        {
        case -1:
            perror("Error in fork");
            exit(-1);

        // In child process
        case 0:
        {
            const int startIndex = forkIndex == 0 ? 0 : rowsPerFork * forkIndex + restRows;
            const int endIndex = rowsPerFork * (forkIndex + 1) + restRows;

            double workerResult = worker(matrix, n, startIndex, endIndex);
            // Write result to pipe
            write(pipes[1], &workerResult, sizeof(double));

            printf("Fork [%d]: pid = [%d] vectors from [%d] to [%d] finished with %f \n", forkIndex + 1, getpid(), startIndex, endIndex, workerResult);
            exit(0);
        }
        }
    }

    for (int forkIndex = 0; forkIndex < forksCount; forkIndex++)
    {
        double workerResult;
        read(pipes[0], &workerResult, sizeof(double));

        if (workerResult)
        {
            if (!result)
            {
                result = workerResult;
            }
            else
            {
                result *= workerResult;
            }
        }
    }

    return result;
}

int main(int argc, char *argv[])
{
    const bool DEBUG = argc < 2;

    if (!DEBUG)
    {
        cout << "You have entered " << argc - 1
             << " arguments:"
             << "\n";

        for (int i = 1; i < argc; ++i)
            cout << argv[i] << "\n";
    }

    // Matrix initializing
    const int MATRIX_N = DEBUG ? 4 : atoi(argv[1]);
    Matrix matrix = getFilledMatrix(MATRIX_N);

    // Detecting the execute mode
    const int MODE = DEBUG ? 1 : atoi(argv[2]);
    const int INSTANCES_COUNT = DEBUG ? 2 : atoi(argv[3]);

    // All average values
    double result = 0;

    switch (MODE)
    {
    case 2:
    {
        const int threadCount = INSTANCES_COUNT;
        cout << "Parallel worker [thread] with " << threadCount << " threads\n\n";
        result = threadWorker(matrix, MATRIX_N, INSTANCES_COUNT);
        break;
    }
    case 1:
    {
        const int forkCount = INSTANCES_COUNT;
        cout << "Parallel worker [fork] with " << forkCount << " forks\n\n";
        result = forkWorker(matrix, MATRIX_N, forkCount);
        break;
    }

    case 0:
    default:
    {
        cout << "Serial worker\n\n";
        result = serialWorker(matrix, MATRIX_N);
    }
    }

    cout << "Average values multiplying result:  ";
    if (result != 0)
    {
        cout << result << '\n';
    }
    else
    {
        cout << "No elements with value = 0.\n\n\n\n";
    }

    return 0;
}
