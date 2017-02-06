//
//  Perceptron.cpp
//  Assignment 1
//
//  Created by Wai Man Chan on 28/9/2015.
//
//

#include "CVPerceptron.h"
#include <Accelerate/Accelerate.h>

CVPerceptron createPerceptron(stepFunctionType stepFunction, int size) {
    //Randomize the weights and deducible
    CVPerceptron perceptron;
    perceptron._weights = malloc(sizeof(sourceVectorType)*size);
    perceptron.vectorSize = size;
    for (int i = 0; i < size; i++) {
        perceptron._weights[i] = rand()/(sourceVectorType)RAND_MAX;
    }
    perceptron._deducible = rand()/(sourceVectorType)RAND_MAX;
    perceptron.trainingRate = 0.000001;
    return perceptron;
};

int updatePerceptron(CVPerceptron perceptron, const char *buffer, int bufferSize) {
    //Package
    //First value: version
    char version = buffer[0];
    int offset = 1;
    int newSize;
    bcopy( buffer+offset, &perceptron.vectorSize, sizeof(int) );
    offset += sizeof(int);
    switch (version) {
        case 0: {
            //Size check
            if (bufferSize != 1+sizeof(int)+sizeof(sourceVectorType)+sizeof(sourceVectorType)*newSize+sizeof(sourceVectorType) )
                return 1;
            
            //Read training rate
            bcopy(buffer+offset, &perceptron.trainingRate, sizeof(sourceVectorType));
            offset += sizeof(sourceVectorType);
            
            //Read weight
            if (perceptron.vectorSize != newSize) {
                free(perceptron._weights);
                perceptron._weights = malloc( sizeof(sourceVectorType)*newSize );
            }
            bcopy(buffer+offset, perceptron._weights, sizeof(sourceVectorType)*newSize );
            offset += sizeof(sourceVectorType) * newSize;
            
            //Read deducible
            bcopy(buffer+offset, &perceptron._deducible, sizeof(sourceVectorType) );
        }
            return 0;
            break;
            
        default:
            return 2;
            break;
    }
}

void score(CVPerceptron perceptron, const sourceVectorType *inputs[], sourceVectorType *result, unsigned long long inputSize) {
    //Accelerate-d
    vDSP_mmul(*inputs, 1, perceptron._weights, 1, result, 1, inputSize, 1, perceptron.vectorSize);
    sourceVectorType tmp = perceptron._deducible*-1;
    vDSP_vsadd(result, 1, &tmp, result, 1, inputSize);
}

void delta(CVPerceptron perceptron, const sourceVectorType *inputs[], const sourceVectorType *result, const sourceVectorType *targetOutput, sourceVectorType *delta, unsigned long long inputSize) {
    //Accelerate-d
    score(perceptron, inputs, delta, inputSize);
    vDSP_vsub(targetOutput, 1, delta, 1, delta, 1, inputSize);
}

const char *deltaPacket(CVPerceptron perceptron, const sourceVectorType *inputs[], const sourceVectorType *result, const sourceVectorType *targetOutput, unsigned long long inputSize, size_t *packSize) {
    *packSize = 1+sizeof(unsigned long long)+sizeof(sourceVectorType)*perceptron.vectorSize+sizeof(sourceVectorType);
    sourceVectorType *deltaVal = malloc( sizeof(sourceVectorType) * inputSize );
    delta(perceptron, inputs, result, targetOutput, deltaVal, inputSize);
    sourceVectorType *weightDelta = malloc( sizeof(sourceVectorType) * perceptron.vectorSize );
    vDSP_mmul(deltaVal, 1, inputs[0], 1, weightDelta, 1, 1, perceptron.vectorSize, inputSize);
    float sizeF = inputSize;
    vDSP_vsdiv(weightDelta, 1, &sizeF, weightDelta, 1, perceptron.vectorSize);
    float debutibleDelta;
    vDSP_meanv(deltaVal, 1, &debutibleDelta, inputSize);
    //Package
    char *buffer = malloc( sizeof(char)* *packSize);
    buffer[0] = 0;
    bcopy(inputSize, &(buffer[1]), sizeof(unsigned long long) );
    bcopy(deltaVal, &(buffer[1+sizeof(unsigned long long)]), sizeof(sourceVectorType)*inputSize );
    bcopy(&debutibleDelta, &(buffer[1+sizeof(unsigned long long)+sizeof(sourceVectorType)*inputSize]), sizeof(sourceVectorType));
    return buffer;
}

void train(CVPerceptron perceptron, const sourceVectorType *inputs[], const sourceVectorType *expectedResults, unsigned long long inputSize) {
    
}
