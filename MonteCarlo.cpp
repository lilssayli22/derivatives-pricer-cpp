//
// Created by ILYASS on 03/09/2026.
//


#include "MonteCarlo.h"

#include <locale>
#include <cmath>
#include <vector>


double MonteCarlo::price() {
    std::vector<double> payoffs;
    for (int i=0 ;i<M;i++) {
        std::vector<std::vector<double>> x = model.asset( option.T,option.N);
        payoffs.push_back( option.payoff(x) );
    }
    double result =0;
    for (int i=0 ;i<M;i++) {
        result += payoffs[i];
    }
    result = (result/M)*std::exp(-option.T*model.r);
    return result;

}
