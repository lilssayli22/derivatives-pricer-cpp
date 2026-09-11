#include "Option.h"

#include <cmath>

int main() {
    Option option;
    option.T = 1.0;
    option.N = 1;
    option.D = 1;
    option.Strike = 100.0;
    option.lambda = pnl_vect_create_from_list(1, 1.0);
    option.optionType = "basket";
    const double values[] = {100.0, 120.0};
    PnlMat* path = pnl_mat_create_from_ptr(2, 1, values);
    const bool success = std::abs(option.payoff(path) - 20.0) ==0;
    pnl_mat_free(&path);
    pnl_vect_free(&option.lambda);
    return success ? 0 : 1;
}
