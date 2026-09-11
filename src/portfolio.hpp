#pragma once

#include <list>
#include <nlohmann/json.hpp>
#include "MonteCarlo.h"

class Position {
public:
    int date;
    PnlVect *deltas;
    PnlVect *deltasStdDev;
    double price;
    double priceStdDev;
    double portfolioValue;

    Position(int date, double price, double priceStdDev, PnlVect* deltas, PnlVect* deltasStdDev, double portfolioValue);
    friend void to_json(nlohmann::json &j, const Position &position);
};

class Portfolio {
public:
    MonteCarlo &monteCarlo;
    std::list<Position> positions;

    Portfolio(nlohmann::json &jsonParams, MonteCarlo &monteCarlo);
    friend std::ostream& operator<<(std::ostream& stm, const Portfolio &portfolio);
    double deltaHedging(PnlMat* Path, int H);
};
