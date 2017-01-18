//
//  Perceptron.hpp
//  Assignment 1
//
//  Created by Wai Man Chan on 28/9/2015.
//
//

#ifndef Perceptron_hpp
#define Perceptron_hpp

typedef double (*stepFunctionType)(double);

#define sourceVectorType double*

class CVPerceptron {
    double trainingRate = 0.000001
    sourceVectorType _weights;
    const stepFunctionType _stepFunction;
    double _deducible;
public:
    CVPerceptron(stepFunctionType stepFunction);
    double score(sourceVectorType input);
    void train(sourceVectorType input, double expectedResult);
};

#endif /* Perceptron_hpp */
