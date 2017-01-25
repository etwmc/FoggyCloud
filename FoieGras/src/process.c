#include "process.h"
#include <stdio.h>

long long _counter(const long long* weight, long long size) {
    long long count = 0;
    for (long long i = 0; i < size; i++)
        count += weight[i];
    return count;
}

void _weightDivider(const long long* weight, long long size, long count, double* relativeWeight) {
    for (long long i = 0; i < size; i++) {
        relativeWeight[i] = (double)weight[i]/(double)count;
    }
}

void _avgSum(const long long* weight, const double* value, long long dataLen, long long size, double* result) {
    long long count = counter(weight, size);
    double *relWeight = malloc(sizeof(double)*size);
    weightDivider(weight, size, count, relWeight);
    for (long long j = 0; j < dataLen; j++) {
        result[j] = 0.0;
    }
    for (long long i = 0; i < size; i++) {
        long long k = i*dataLen;

        for (long long j = 0; j < dataLen; j++) {
            result[j] += relWeight[i]*value[k+j];
        }
    }
    free(relWeight);
}

void _vecDScale(double *vector, long long size, double scale) {
    for (long long i = 0; i < size; i++) {
        vector[i] *= scale;
    }
}

void _vecDScaleThenAdd(const double *a, const double *b, double c, double *ans, long long size) {
    for (long long i = 0; i < size; i++) {
        ans[i] = a[i] + b[i] * c;
    }
}

long long counter(const void* weight, long long size) { return _counter(weight, size); }

void weightDivider(const void* weight, long long size, long long count, void* relativeWeight) { _weightDivider(weight, size, count, relativeWeight); }

void avgSum(const void* weight, const void* value, long long dataLen, long long size, void* result) { _avgSum(weight, value, dataLen, size, result); }

void vecDScale(void *vector, long long size, double scale) { _vecDScale(vector, size, scale); }

void vecDScaleThenAdd(const void *a, const void *b, double c, void *ans, long long size) { _vecDScaleThenAdd(a, b, c, ans, size); }