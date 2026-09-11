#include "BlackScholesModel.h"

#include <cmath>

int main()
{
    PnlVect *sigma = pnl_vect_create_from_list(1, 0.0);
    PnlVect *S0 = pnl_vect_create_from_list(1, 100.0);

    BlackScholesModel model(1, 0.05, 0.0, sigma, S0);

    PnlRng *rng = pnl_rng_create(PNL_RNG_MERSENNE);
    pnl_rng_sseed(rng, 42);

    PnlMat *path = pnl_mat_new();
    model.asset(1.0, 1, path, rng);

    const double expected = 100.0 * std::exp(0.05);

    const bool success =
        path->m == 2 &&
        path->n == 1 &&
        std::abs(MGET(path, 1, 0) - expected) == 0;

    pnl_mat_free(&path);
    pnl_rng_free(&rng);
    pnl_mat_free(&model.L);
    pnl_vect_free(&model.S0);
    pnl_vect_free(&model.sigma);

    return success ? 0 : 1;
}