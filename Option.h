#include <vector>

class Option {
public:
    double T;
    int N;
    int D;
    double K;
    std::vector<double> lambda;

    double payoff(const std::vector<std::vector<double>>& path);
};