//
//  Perceptron.cpp
//  Assignment 1
//
//  Created by Wai Man Chan on 28/9/2015.
//
//

#include "Perceptron.hpp"
#include <stdlib.h>
#include <time.h>
#include "VectorSpace.hpp"

template<> CVPerceptron<VectorSpace::vectorType_9D>::CVPerceptron(stepFunctionType stepFunction): _stepFunction(stepFunction) {
    //Randomize the weights and deducible
    for (int i = 0; i < _weights.size(); i++) {
        _weights.data[i] = rand()/(double)RAND_MAX;
    }
    _deducible = rand()/(double)RAND_MAX;
};

template<> double CVPerceptron<VectorSpace::vectorType_9D>::score(VectorSpace::vectorType_9D input) {
    return _stepFunction(input.dotProduct(_weights)-_deducible);
}

template<> void CVPerceptron<VectorSpace::vectorType_9D>::train(VectorSpace::vectorType_9D input, double expectedResult) {
    double actualResult = score(input);
    if (expectedResult != actualResult) {
        VectorSpace::vectorType_9D weightDelta = input * (trainingRate * (expectedResult - actualResult));
        _weights = _weights + weightDelta;
        _deducible += (expectedResult - actualResult) * trainingRate;
    }
}
