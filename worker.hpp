#include <iostream>
#include <vector>
#include <unistd.h>
#include <numeric>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <queue> // the message queue

using namespace std;

typedef vector<int> Vector;
typedef vector<Vector> Matrix;

template <class t>
void printVector(t v)
{
    if (v.empty())
    {
        cout << "Vector is empty!\n";
        return;
    }

    for (int i = 0; i < v.size(); i++)
        cout << v.at(i) << " ";
}

void printMatrix(Matrix matrix, int n)
{
    if (n > 50)
    {
        cout << "Matrix too big to display." << endl;
        return;
    }

    cout << "Matrix:" << endl;

    for (int i = 0; i < n; i++)
    {

        printVector(matrix[i]);
        cout << endl;
    }
}

Matrix getFilledMatrix(int n)
{
    Matrix matrix(n, Vector(n));

    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
        {
            matrix[i].at(j) = rand() % n;
        }

    // dublicate columnst to rows
    printMatrix(matrix, n);
    printf("\n");

    for (int j = 0; j < n; ++j)
    {
        Vector column;
        for (int i = 0; i < n; ++i)
        {
            column.push_back(matrix[i][j]);
        }
        matrix.push_back(column);
    }

    return matrix;
}

Vector getZeroValuedElementsIndexes(Vector v, int j)
{
    Vector indexes;
    for (int i = 0; i < v.size(); i++)
    {
        if (v[i] == 0)
        {
            indexes.push_back(j);
            indexes.push_back(i);
        }
    }

    return indexes;
}

double getAverageValue(Vector v)
{
    const int size = v.size();
    if (size == 0)
    {
        return 0;
    }

    return accumulate(v.begin(), v.end(), 0.0) / size;
}

double worker(Matrix matrix, int n, const int startIndex, const int endIndex)
{
    double result = 0;

    for (int i = startIndex; i < endIndex; i++)
    {
        bool isRow = i < n;
        int index = isRow ? i : i - n;
        Vector vector = matrix[i];

        Vector zeroValuedElementsIndexes = getZeroValuedElementsIndexes(vector, index);
        if (zeroValuedElementsIndexes.size())
        {

            double averageValue = getAverageValue(zeroValuedElementsIndexes);
            if (!result)
            {
                result = averageValue;
            }
            else
            {
                result *= averageValue;
            }

            // printf("%s [%d] average value = %f.\n", isRow ? "Row" : "Column", index, averageValue);
        }
    }

    return result;
}

struct thread_params
{
    Matrix matrix;
    int n;
    vector<float> &avgResults;
    int startIndex;
    int endIndex;
};

// structure for mthreads arguments
typedef struct
{
    Matrix &matrix;
    const int n;
    const int startIndex;
    const int endIndex;
    queue<double> &queue;
} worker_context;

// structure for message queue
struct mesg_buffer
{
    double workerResult;
} message;

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

void *workerThreadWrapper(void *context)
{

    // get args from context
    worker_context *workerArgs = (worker_context *)context;

    // perform work
    // double workerResult  = 0;
    double workerResult = worker(workerArgs->matrix, workerArgs->n, workerArgs->startIndex, workerArgs->endIndex);

    // get thread Id of calling thread
    pthread_t thId = pthread_self();
    printf("Thread [%ld]:  vectors from [%d] to [%d] finished with value: [%f]\n", (long)thId, workerArgs->startIndex, workerArgs->endIndex, workerResult);

    if (workerResult != 0)
    {
        pthread_mutex_lock(&mutex1);
        workerArgs->queue.push(workerResult);
        pthread_mutex_unlock(&mutex1);
    }

    delete workerArgs;

    return NULL;
}
