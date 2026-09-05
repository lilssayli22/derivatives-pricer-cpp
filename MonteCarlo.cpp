//
// Created by ILYASS on 03/09/2026.
//


#include "MonteCarlo.h"

#include <locale>


double MonteCarlo ::moyenner(std :: vector <double> x) {
    double result = 0.0;
    for(std::vector<int>::iterator it = v.begin(); it != v.end(); it++) {
        result = result + x[*it];
    }
    return result;
}