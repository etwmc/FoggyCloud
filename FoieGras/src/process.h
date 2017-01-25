#include <stdlib.h>

long long counter(const void* weight, long long size);

void weightDivider(const void* weight, long long size, long long count, void* relativeWeight);

void avgSum(const void* weight, const void* value, long long dataLen, long long size, void* result);

void vecDScale(void *vector, long long size, double scale);

//ans = a + b * c
void vecDScaleThenAdd(const void *a, const void *b, double c, void *ans, long long size);