//
// Created by ILYASS on 04/09/2026.
//

#include "BlackScholesModel.h"
#include <cmath>
static const double EPS = 1E-10;

BlackScholesModel::BlackScholesModel(int D, double r, double rho,
                                      PnlVect* sigma, PnlVect* S0) {
    this->D = D;
    this->r = r;
    this->rho = rho;
    this->sigma = pnl_vect_copy(sigma);
    this->S0 = pnl_vect_copy(S0);

    this->L = pnl_mat_create_from_scalar(D, D, rho);
    for (int i = 0; i < D; i++) MLET(this->L, i, i) = 1.0;
    pnl_mat_chol(this->L);}

    
void BlackScholesModel::asset(double T, int N,PnlMat* simulation,PnlRng* rng) {

    PnlMat* gaussian = generate_gaussianMat(N, rng);

    pnl_mat_resize(simulation, N + 1, D);
    pnl_mat_set_row(simulation, S0, 0);

    double dt = T / N;

    for (int i = 0; i < N; i++) {
        simule_etape(simulation,  i, simulation,  gaussian,    i,  i + 1, dt);
    }

    pnl_mat_free(&gaussian);
}

int BlackScholesModel:: compute_last_index(double t, double T, int N) {
    double dt = T / N;
    int nearest_index = std::round(t / dt);
    if (std::fabs(nearest_index * dt - t) < EPS) {
        return nearest_index;
    } else {
        return int(t / dt);
    }
}


void BlackScholesModel::asset(PnlMat* past, double t, double T, int N,
                              PnlMat* simulation, PnlRng* rng) {

    int last_index = compute_last_index(t, T, N);

    pnl_mat_resize(simulation, N + 1, D);

    for (int i = 0; i <= last_index; i++) {
        for (int d = 0; d < D; d++) {
            MLET(simulation, i, d) = MGET(past, i, d);
        }
    }
    if (last_index >= N) {
      return;
  }

    double dt = T / N;

    PnlMat* gaussian = generate_gaussianMat(N - last_index, rng);

    double premier_pas = (last_index + 1) * dt - t;

    simule_etape(
        past,
        past->m - 1,
        simulation,
        gaussian,
        0,                  // premier vecteur gaussien
        last_index + 1,     // destination
        premier_pas
    );

    for (int i = last_index + 1; i < N; i++) {
        simule_etape(
            simulation,
            i,
            simulation,
            gaussian,
            i - last_index,
            i + 1,
            dt
        );
    }

    pnl_mat_free(&gaussian);
}

void BlackScholesModel::shift_asset(
      PnlMat* shifted,
      PnlMat* path,
      int d,
      double h,
      double t,
      double T,
      int N
  ) {
      const int last_index = compute_last_index(t, T, N);
      const double dt = T / N;

      int first_idx = last_index + 1;
      if (std::fabs(t - last_index * dt) < EPS) {
          first_idx = last_index;
      }

      for (int i = first_idx; i <= N; i++) {
          MLET(shifted, i, d) =
              MGET(path, i, d) * (1.0 + h);
      }
  }

PnlMat* BlackScholesModel::generate_gaussianMat(int N, PnlRng* rng) {
    PnlMat* gaussian = pnl_mat_create(N, D);

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < D; j++) {
            MLET(gaussian, i, j) = pnl_rng_normal(rng);
        }
    }
    return gaussian;
}

void BlackScholesModel::simule_etape(const PnlMat* source,int source_index,PnlMat* simulation,const PnlMat* gaussian,int gaussian_index,int destination_index,double pas) {


    for (int d = 0; d < D; d++) {

        double produit = 0.0;

        // Calcul de (L * G)_d
        for (int k = 0; k <= d; k++) {
            produit += MGET(L, d, k) *
                       MGET(gaussian, gaussian_index, k);
        }

        double precedent = MGET(source, source_index, d);
        const double sigmaD = GET(sigma, d);

        MLET(simulation, destination_index, d) =
              precedent *
              std::exp(
                  (r - sigmaD * sigmaD / 2.0) * pas
                  + sigmaD * std::sqrt(pas) * produit
              );
    }
}


