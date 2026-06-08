#include <algorithm>
#include <chrono>
#include <functional>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace std;

using Matrix = vector<vector<int>>;

void runBenchmark();
Matrix generateMatrix(int size, int seed);
Matrix multiplySingleThread(const Matrix& A, const Matrix& B);
Matrix multiplyWithThreads(const Matrix& A, const Matrix& B, int requestedThreadCount, int& usedThreadCount);
void computeRows(const Matrix& A, const Matrix& B, Matrix& C, int startRow, int endRow);
double measureMilliseconds(const function<void()>& operation);
bool areMatricesEqual(const Matrix& A, const Matrix& B);
long long calculateChecksum(const Matrix& M);
string getMatrixSample(const Matrix& M);
vector<int> getThreadCounts(int matrixSize);
void printBenchmarkHeader();
void printBenchmarkRow(const string& methodName, int threadCount, double elapsedMs, const string& resultText,
                       double speedup, long long checksum);

int main() {
    runBenchmark();
    return 0;
}

void runBenchmark() {
    vector<int> matrixSizes = {300, 500, 700};
    unsigned int hardwareThreads = thread::hardware_concurrency();

    cout << endl;
    cout << "CPU loginiu branduoliu skaicius: ";

    if (hardwareThreads == 0) {
        cout << "Nepavyko nustatyti." << endl;
    } else {
        cout << hardwareThreads << endl;
    }

    for (int matrixSize : matrixSizes) {
        cout << endl;
        cout << "Testuojamas matricos dydis: " << matrixSize << "x" << matrixSize << endl;
        cout << "---------------------------------------------------" << endl;

        Matrix A = generateMatrix(matrixSize, 100);
        Matrix B = generateMatrix(matrixSize, 200);

        Matrix singleThreadResult;
        double singleThreadTime = measureMilliseconds([&]() {
            singleThreadResult = multiplySingleThread(A, B);
        });

        long long singleThreadChecksum = calculateChecksum(singleThreadResult);

        cout << "Atsakymo pavyzdys pagal viengiji metoda:" << endl;
        cout << getMatrixSample(singleThreadResult) << endl;
        cout << "Kontroline suma: " << singleThreadChecksum << endl;
        cout << endl;

        printBenchmarkHeader();
        printBenchmarkRow("Viengijis", 1, singleThreadTime, "Teisingas", 1.0, singleThreadChecksum);

        for (int requestedThreadCount : getThreadCounts(matrixSize)) {
            Matrix threadedResult;
            int usedThreadCount = 0;

            double threadedTime = measureMilliseconds([&]() {
                threadedResult = multiplyWithThreads(A, B, requestedThreadCount, usedThreadCount);
            });

            bool isCorrect = areMatricesEqual(singleThreadResult, threadedResult);
            double speedup = singleThreadTime / threadedTime;
            long long checksum = calculateChecksum(threadedResult);

            printBenchmarkRow("thread", usedThreadCount, threadedTime, isCorrect ? "Teisingas" : "Neteisingas",
                              speedup, checksum);
        }
    }

    cout << endl;
    cout << "Testavimas baigtas." << endl;
}

Matrix generateMatrix(int size, int seed) {
    mt19937 generator(seed);
    uniform_int_distribution<int> distribution(1, 9);
    Matrix matrix(size, vector<int>(size));

    for (int row = 0; row < size; row++) {
        for (int column = 0; column < size; column++) {
            matrix[row][column] = distribution(generator);
        }
    }

    return matrix;
}

Matrix multiplySingleThread(const Matrix& A, const Matrix& B) {
    Matrix result(A.size(), vector<int>(B[0].size(), 0));
    computeRows(A, B, result, 0, static_cast<int>(A.size()));
    return result;
}

Matrix multiplyWithThreads(const Matrix& A, const Matrix& B, int requestedThreadCount, int& usedThreadCount) {
    Matrix result(A.size(), vector<int>(B[0].size(), 0));

    usedThreadCount = min(max(1, requestedThreadCount), static_cast<int>(A.size()));

    vector<thread> threads;
    threads.reserve(usedThreadCount);

    int baseRowsPerThread = static_cast<int>(A.size()) / usedThreadCount;
    int extraRows = static_cast<int>(A.size()) % usedThreadCount;
    int startRow = 0;

    for (int i = 0; i < usedThreadCount; i++) {
        int rowsForThisThread = baseRowsPerThread;

        if (i < extraRows) {
            rowsForThisThread++;
        }

        int endRow = startRow + rowsForThisThread;

        threads.emplace_back(computeRows, cref(A), cref(B), ref(result), startRow, endRow);

        startRow = endRow;
    }

    for (thread& t : threads) {
        t.join();
    }

    return result;
}

void computeRows(const Matrix& A, const Matrix& B, Matrix& C, int startRow, int endRow) {
    int colsB = static_cast<int>(B[0].size());
    int common = static_cast<int>(B.size());

    for (int row = startRow; row < endRow; row++) {
        for (int column = 0; column < colsB; column++) {
            int sum = 0;

            for (int k = 0; k < common; k++) {
                sum += A[row][k] * B[k][column];
            }

            C[row][column] = sum;
        }
    }
}

double measureMilliseconds(const function<void()>& operation) {
    auto start = chrono::high_resolution_clock::now();

    operation();

    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> elapsed = end - start;

    return elapsed.count();
}

bool areMatricesEqual(const Matrix& A, const Matrix& B) {
    if (A.size() != B.size()) {
        return false;
    }

    for (size_t row = 0; row < A.size(); row++) {
        if (A[row].size() != B[row].size()) {
            return false;
        }

        for (size_t column = 0; column < A[row].size(); column++) {
            if (A[row][column] != B[row][column]) {
                return false;
            }
        }
    }

    return true;
}

long long calculateChecksum(const Matrix& M) {
    long long sum = 0;

    for (const vector<int>& row : M) {
        for (int value : row) {
            sum += value;
        }
    }

    return sum;
}

string getMatrixSample(const Matrix& M) {
    int size = static_cast<int>(M.size());
    int middle = size / 2;

    stringstream ss;
    ss << "[0,0]=" << M[0][0]
       << ", [" << middle << "," << middle << "]=" << M[middle][middle]
       << ", [" << size - 1 << "," << size - 1 << "]=" << M[size - 1][size - 1];

    return ss.str();
}

vector<int> getThreadCounts(int matrixSize) {
    vector<int> threadCounts = {2, 4};
    unsigned int hardwareThreads = thread::hardware_concurrency();

    if (hardwareThreads > 0) {
        threadCounts.push_back(static_cast<int>(hardwareThreads));
    }

    for (int& threadCount : threadCounts) {
        threadCount = min(threadCount, matrixSize);
    }

    sort(threadCounts.begin(), threadCounts.end());
    threadCounts.erase(unique(threadCounts.begin(), threadCounts.end()), threadCounts.end());

    return threadCounts;
}

void printBenchmarkHeader() {
    cout << left
         << setw(18) << "Metodas"
         << setw(10) << "Gijos"
         << setw(14) << "Laikas ms"
         << setw(14) << "Rezultatas"
         << setw(16) << "Pagreitejimas"
         << setw(18) << "Kontroline suma"
         << endl;
}

void printBenchmarkRow(const string& methodName, int threadCount, double elapsedMs, const string& resultText,
                       double speedup, long long checksum) {
    stringstream timeText;
    timeText << fixed << setprecision(3) << elapsedMs;

    stringstream speedupText;
    speedupText << fixed << setprecision(2) << speedup << "x";

    cout << left
         << setw(18) << methodName
         << setw(10) << threadCount
         << setw(14) << timeText.str()
         << setw(14) << resultText
         << setw(16) << speedupText.str()
         << setw(18) << checksum
         << endl;
}
