#pragma once


#include <string>
#include <pnl/pnl_matrix.h>
#include <pnl/pnl_vector.h>


class Option {
public:
    double fdStep;
    double T;
    int N;
    int D;
    double Strike;
    PnlVect* lambda;
    std::string optionType;

    double payoff(const PnlMat* path);
};
