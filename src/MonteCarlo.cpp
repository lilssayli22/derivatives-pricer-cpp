//
// Created by ILYASS on 03/09/2026.
//


#include "MonteCarlo.h"

#include <cmath>


std::tuple<double,double> MonteCarlo::price() {
    PnlMat* x = pnl_mat_new();
    double sum = 0.0;
      double sumSquares = 0.0;

    for (int i=0 ;i<M;i++) {
         model.asset(option.T, option.N, x, rng);
         double payoff = option.payoff(x);

          sum += payoff;
          sumSquares += payoff * payoff;
        }

    double espylon;
    double result;
    espylon = std::exp(-option.T * model.r)
        * std::sqrt(sumSquares / M - (sum / M) * (sum / M));
    result = (sum/M)*std::exp(-option.T*model.r);
    
    double priceStdev = espylon / std::sqrt(M);
    
    pnl_mat_free(&x);
    return std::make_tuple( result , priceStdev);

}

std::tuple<double,double> MonteCarlo::price( PnlMat* past, double t) {
    PnlMat* x = pnl_mat_new();
    double sum = 0.0;
      double sumSquares = 0.0;
    for (int i=0 ;i<M;i++) {
        model.asset(past, t, option.T, option.N, x, rng);
        double payoff = option.payoff(x);

          sum += payoff;
          sumSquares += payoff * payoff;
    }
    double espylon;
    double result;
    espylon = std::exp(-(option.T - t) * model.r)
        * std::sqrt(sumSquares / M - (sum / M) * (sum / M));
    result = (sum/M)*std::exp(-(option.T - t)*model.r);

    double priceStdev = espylon / std::sqrt(M);

    pnl_mat_free(&x);
    return std::make_tuple( result , priceStdev);
}
 std::tuple<PnlVect*, PnlVect*> MonteCarlo::delta(
      PnlMat* past,
      double t
  ) {
      const int D = model.D;

      PnlVect* sum = pnl_vect_create_from_zero(D);
      PnlVect* sumSquares = pnl_vect_create_from_zero(D);

      PnlMat* path = pnl_mat_new();
      PnlMat* shiftedUp = pnl_mat_new();
      PnlMat* shiftedDown = pnl_mat_new();

      for (int simulationIndex = 0;
           simulationIndex < M;
           simulationIndex++) {

          model.asset(
              past,
              t,
              option.T,
              option.N,
              path,
              rng
          );

          /*
           * On copie la trajectoire seulement deux fois par simulation :
           * une pour le bump positif, une pour le négatif.
           */
          pnl_mat_clone(shiftedUp, path);
          pnl_mat_clone(shiftedDown, path);

          for (int d = 0; d < D; d++) {
              model.shift_asset(
                  shiftedUp,
                  path,
                  d,
                  option.fdStep,
                  t,
                  option.T,
                  option.N
              );

              model.shift_asset(
                  shiftedDown,
                  path,
                  d,
                  -option.fdStep,
                  t,
                  option.T,
                  option.N
              );

              const double payoffUp = option.payoff(shiftedUp);
              const double payoffDown = option.payoff(shiftedDown);

              const double spot =
                  MGET(past, past->m - 1, d);

              const double deltaSample =
                  (payoffUp - payoffDown)
                  / (2.0 * spot * option.fdStep);

              LET(sum, d) += deltaSample;
              LET(sumSquares, d) += deltaSample * deltaSample;

              /*
               * Remet la colonne d dans son état original.
               * Les matrices shiftedUp et shiftedDown sont prêtes
               * pour l'actif d + 1.
               */
              model.shift_asset(
                  shiftedUp,
                  path,
                  d,
                  0.0,
                  t,
                  option.T,
                  option.N
              );

              model.shift_asset(
                  shiftedDown,
                  path,
                  d,
                  0.0,
                  t,
                  option.T,
                  option.N
              );
          }
      }

      const double discount =
          std::exp(-(option.T - t) * model.r);

      PnlVect* deltas = pnl_vect_create_from_zero(D);
      PnlVect* deltaStdDev = pnl_vect_create_from_zero(D);

      for (int d = 0; d < D; d++) {
          const double mean = GET(sum, d) / M;

          const double variance =
              GET(sumSquares, d) / M - mean * mean;

          LET(deltas, d) = discount * mean;
          LET(deltaStdDev, d) =
              discount * std::sqrt(variance / M);
      }

      pnl_vect_free(&sum);
      pnl_vect_free(&sumSquares);

      pnl_mat_free(&path);
      pnl_mat_free(&shiftedUp);
      pnl_mat_free(&shiftedDown);

      return std::make_tuple(deltas, deltaStdDev);
  }