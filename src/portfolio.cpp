#include <iostream>
#include <tuple>
#include "portfolio.hpp"
#include "json_helper.hpp"

Position::Position(int date, double price, double priceStdDev, PnlVect* deltas, PnlVect* deltasStdDev, double portfolioValue)
    : date(date), price(price), priceStdDev(priceStdDev), portfolioValue(portfolioValue), deltas(deltas), deltasStdDev(deltasStdDev) {
}

void to_json(nlohmann::json &j, const Position &position) {
    j["date"] = position.date;
    j["value"] = position.portfolioValue;
    j["price"] = position.price;
    j["priceStdDev"] = position.priceStdDev;
    j["deltas"] = position.deltas;
    j["deltasStdDev"] = position.deltasStdDev;
}

Portfolio::Portfolio(nlohmann::json &jsonParams, MonteCarlo &monteCarlo)
    : monteCarlo(monteCarlo) {
}

std::ostream& operator<<(std::ostream& o, const Portfolio &portfolio) {
    nlohmann::json j = {
        {"portfolio", portfolio.positions}
    };
    o << std::setw(4) << j << std::endl;
    return o;
}

double Portfolio::deltaHedging(PnlMat* Path, int H) {
    const int N = monteCarlo.option.N;
    const int D = monteCarlo.option.D;
    const double hdgTime = monteCarlo.option.T / H;   // T doit etre divisible par H
    const int step = H / N;
    const double growth = std::exp(monteCarlo.model.r * hdgTime);  // invariant de boucle
  PnlVect* spots_i = pnl_vect_create(D);
  pnl_mat_get_row(spots_i, Path, 0);
    const std::tuple<double, double> initialPrice = monteCarlo.price();
    const double p0 = std::get<0>(initialPrice);
    const double ic0 = std::get<1>(initialPrice);
    PnlMat* past0 = pnl_mat_create(1, D);
  pnl_mat_set_row(past0, spots_i, 0);

  std::tuple<PnlVect*, PnlVect*> initialDelta =
      monteCarlo.delta(past0, 0.0);

  pnl_mat_free(&past0);
    PnlVect* delta_prev = std::get<0>(initialDelta);
    PnlVect* s = std::get<1>(initialDelta);

    
    double Vi = p0 - pnl_vect_scalar_prod(delta_prev, spots_i);

    positions.emplace_back(0, p0, ic0, pnl_vect_copy(delta_prev), pnl_vect_copy(s), Vi);
    pnl_vect_free(&s);

    PnlMat* past = pnl_mat_new();
    pnl_mat_resize(past, N + 1, D);
    pnl_mat_set_row(past, spots_i, 0);

    PnlVect* tempo = pnl_vect_create(D);

    // boucle principale de couverture
    for (int i = 1; i <= H; i++) {
        const int k = i / step;
        pnl_mat_resize(past, k + 2, D);
        for (int j = 0; j <= k; j++) {
            pnl_mat_get_row(spots_i, Path, j * step);
            pnl_mat_set_row(past, spots_i, j);
        }

        pnl_mat_get_row(spots_i, Path, i);
        pnl_mat_set_row(past, spots_i, k + 1);

        const double t = i * hdgTime;
        std::tuple<PnlVect*, PnlVect*> currentDelta = monteCarlo.delta(past, t);
        PnlVect* delta_cur = std::get<0>(currentDelta);
        PnlVect* tmp = std::get<1>(currentDelta);
        const std::tuple<double, double> currentPrice = monteCarlo.price(past, t);
        const double price = std::get<0>(currentPrice);
        const double stdPrice = std::get<1>(currentPrice);

        pnl_vect_clone(tempo, delta_cur);
        pnl_vect_minus_vect(tempo, delta_prev);
        Vi = Vi * growth - pnl_vect_scalar_prod(tempo, spots_i);

        positions.emplace_back(i, price, stdPrice, pnl_vect_copy(delta_cur), pnl_vect_copy(tmp), Vi);

        pnl_vect_free(&tmp);
        pnl_vect_free(&delta_prev);
        delta_prev = delta_cur;
    }

    pnl_mat_resize(past, N + 1, D);
    const double pnl = Vi + pnl_vect_scalar_prod(delta_prev, spots_i) - monteCarlo.option.payoff(past);

    pnl_vect_free(&delta_prev);
    pnl_vect_free(&spots_i);
    pnl_vect_free(&tempo);
    pnl_mat_free(&past);
    return pnl;
}
