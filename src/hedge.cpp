#include "MonteCarlo.h"
#include "portfolio.hpp"

#include <iostream>
#include <algorithm>
#include <string>
#include <fstream>
#include <stdexcept>
#include <tuple>
#include "Option.h"
#include "MonteCarlo.h"
#include "json_helper.hpp"
#include "BlackScholesModel.h"
#include "pricing_results.hpp"


PnlVect* vectReader(const nlohmann::json &optionParams , const std::string &key, int D){
    PnlVect* vect = optionParams.at(key).get<PnlVect*>();
    if (D > 1 && vect->size == 1) {
        double valeur = GET(vect, 0);
        pnl_vect_free(&vect);
        vect = pnl_vect_create_from_scalar(D, valeur);
    }
    if (vect->size != D) {
        pnl_vect_free(&vect);
        throw std::invalid_argument("Invalid size for " + key);
    }
    return vect;
}

int main(int argc , char ** argv) {
    if (argc < 3) {
        std::cerr << "Missing JSON file path" << std::endl;
        return 1;
    }

    std::string jsonPath = argv[2];
    

    std::ifstream file(jsonPath);
    if (!file.is_open()) {
        std::cerr << "probleme d'ouverture" << std::endl;
        return 1;
    }
    double K = 0.0;
    nlohmann::json paramsOption ;
    file >> paramsOption;
    const int D = paramsOption.at("model size").get<int>();
    const double r = paramsOption.at("interest rate").get<double>();
    const double rho = paramsOption.at("correlation").get<double>();
    const double T = paramsOption.at("maturity").get<double>();
    
    const int N = paramsOption.at("timestep number").get<int>();
    const int M = paramsOption.at("sample number").get<int>();
    const std::string optionType = paramsOption.at("option type").get<std::string>();
    if (optionType == "asian" || optionType == "basket"){
        K = paramsOption.at("strike").get<double>();
    }
    const double fdStep = paramsOption.at("fd step").get<double>();
    const int H = paramsOption.at("hedging dates number").get<int>();

    PnlVect* sigma = vectReader(paramsOption,"volatility",D);
    PnlVect* S0 = vectReader(paramsOption,"spot",D);
    PnlVect *lambda = vectReader(paramsOption, "payoff coefficients", D);

    Option option ;
    option.D = D;
    option.T = T;
    option.N = N;
    option.Strike = K;
    option.lambda = lambda;
    option.optionType = optionType;
    option.fdStep = fdStep;

    MonteCarlo monteCarlo = {
        {D, r, rho, sigma, S0},
        option,
        M,
        pnl_rng_create(PNL_RNG_MERSENNE)
    };
    pnl_rng_sseed(monteCarlo.rng, 40);

    PnlMat* Path = pnl_mat_create_from_file(argv[1]);
    Portfolio portfolio(paramsOption, monteCarlo);
    double finalPnL = portfolio.deltaHedging(Path, H);

    double initialPrice = portfolio.positions.front().price;
    double initialPriceStdDev = portfolio.positions.front().priceStdDev;
    nlohmann::json result = {
        {"finalPnL", finalPnL},
        {"initialPrice", initialPrice},
        {"initialPriceStdDev", initialPriceStdDev}
    };
    std::cout << result << std::endl;

    pnl_mat_free(&Path);
    pnl_rng_free(&monteCarlo.rng);
    pnl_vect_free(&sigma);
    pnl_vect_free(&S0);
    pnl_vect_free(&lambda);

    return 0;
}