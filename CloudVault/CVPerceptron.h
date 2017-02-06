//
//  Perceptron.hpp
//  Assignment 1
//
//  Created by Wai Man Chan on 28/9/2015.
//
//

#ifndef Perceptron_hpp
#define Perceptron_hpp

#include <stdlib.h>
#include <time.h>
#include <string.h>

typedef double (*stepFunctionType)(double);

#define sourceVectorType float

typedef struct CVPerceptron {
    sourceVectorType trainingRate;
    int vectorSize;
    sourceVectorType *_weights;
    const stepFunctionType _stepFunction;
    sourceVectorType _deducible;
} CVPerceptron;

CVPerceptron createPerceptron(stepFunctionType stepFunction, int size);
void score(CVPerceptron perceptron, const sourceVectorType *inputs[], sourceVectorType *result, unsigned long long inputSize);
void delta(CVPerceptron perceptron, const sourceVectorType *inputs[], const sourceVectorType *result, const sourceVectorType *targetOutput, sourceVectorType *delta, unsigned long long inputSize);
const char *deltaPacket(CVPerceptron perceptron, const sourceVectorType *inputs[], const sourceVectorType *result, const sourceVectorType *targetOutput, unsigned long long inputSize, size_t *packSize);
void train(CVPerceptron perceptron, const sourceVectorType *inputs[], const sourceVectorType *expectedResults, unsigned long long inputSize);

#endif /* Perceptron_hpp */
