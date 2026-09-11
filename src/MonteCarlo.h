#pragma once

#include "BlackScholesModel.h"
#include "Option.h"
#include <pnl/pnl_random.h>
#include <tuple>


class MonteCarlo {
public:
    BlackScholesModel model;
    Option option;
    int M;
    PnlRng* rng;

    std::tuple<double,double> price();
    std::tuple<double,double> price( PnlMat* past, double t);
    std::tuple<PnlVect*, PnlVect*> delta(PnlMat* past, double t);

};
