//
// Created by ILYASS on 04/09/2026.
//

#include "BlackScholesModel.h"
#include <random>
#include <cmath>


std::vector<std::vector<double> > BlackScholesModel::asset(double T, int N) {
    std::normal_distribution<double> gauss(0.0, 1.0);
    std::vector<std::vector<double> > gaussian;
    // on genere le vecteur gaussien d abord :
    for (int i=0; i < N; i++) {
        std::vector<double> x;
        for (int j=0; j < D; j++) {
            x.push_back(gauss(this->rng));
        }
        gaussian.push_back(x);
    }
    std::vector<std::vector<double> > simulation;
    simulation.push_back(S0);
    double dt = T / N;

    for (int i = 0; i < N; i++) {
        std::vector<double> ligne(D);
        for (int d = 0; d < D; d++) {
            double produit = 0;
            for (int k = 0; k < D; k++) {
                produit += L[d][k] * gaussian[i][k];
            }
            ligne[d] = simulation[i][d] *
                       std::exp((r - sigma[d]*sigma[d]/2) * dt + sigma[d] * std::sqrt(dt) * produit);
        }
        simulation.push_back(ligne);
    }

    return simulation;


}
