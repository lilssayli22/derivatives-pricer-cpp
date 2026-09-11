#include "MonteCarlo.h"

#include <algorithm>
#include <array>
#include <iomanip>
#include <iostream>
#include <tuple>

#include <pnl/pnl_matrix.h>
#include <pnl/pnl_random.h>
#include <pnl/pnl_vector.h>

int main()
{
    // Paramètres de basket_5d_1.json
    const int D = 5;
    const double r = 0.04879;
    const double rho = 0.4;
    const double T = 3.0;
    const double K = 100.0;
    const int N = 1;
    const int M = 50000;
    const int H = 1000;
    const double fdStep = 0.1;

    constexpr double confidenceFactor = 1.96;

    const std::array<int, 6> dates = {
        7, 8, 9, 10, 11, 12
    };

    const std::array<std::array<double, 5>, 6>
        expectedDeltas = {{
            {
                0.14863778971957106,
                0.1484827775066498,
                0.14883245870113473,
                0.14832267656037104,
                0.14879707159967215
            },
            {
                0.14824884640979397,
                0.14762736419974937,
                0.14796793050873844,
                0.14749419689034338,
                0.1480850459912539
            },
            {
                0.14875209734909678,
                0.14870614025473583,
                0.1487959277468368,
                0.14825225683590673,
                0.14908000022099094
            },
            {
                0.14898093913916116,
                0.1489742187315097,
                0.14879603688862897,
                0.1488409978685686,
                0.1489373048796794
            },
            {
                0.15114349599230595,
                0.15088604282662255,
                0.15106713931370502,
                0.15050524427526582,
                0.15134620769186632
            },
            {
                0.1489107603450451,
                0.14853181246187966,
                0.14849853415285655,
                0.14844303742887627,
                0.14855946862990527
            }
        }};

    const std::array<std::array<double, 5>, 6>
        expectedDeltaStdDevs = {{
            {
                0.0005365799251331428,
                0.0005361025381483798,
                0.0005386639431830773,
                0.00053679378620791,
                0.0005373039149473934
            },
            {
                0.0005367140261504072,
                0.0005345463335152524,
                0.0005373406644684078,
                0.0005354698014236959,
                0.0005363872312411901
            },
            {
                0.0005348023373117473,
                0.0005340955630286953,
                0.0005346530057363136,
                0.0005332332154189311,
                0.0005358124487919375
            },
            {
                0.0005358339474902742,
                0.0005363297963361162,
                0.0005352940158481453,
                0.000535856149145204,
                0.0005366195782323006
            },
            {
                0.0005316382959468851,
                0.0005298455630412839,
                0.0005324145244996132,
                0.0005296428212628925,
                0.0005336104763874323
            },
            {
                0.0005386345889727511,
                0.000537726431361341,
                0.000536456888678628,
                0.000537264327311302,
                0.0005373676293824876
            }
        }};

    PnlVect* sigma =
        pnl_vect_create_from_scalar(D, 0.2);

    PnlVect* S0 =
        pnl_vect_create_from_scalar(D, 100.0);

    PnlVect* lambda =
        pnl_vect_create_from_scalar(D, 0.2);

    Option option;
    option.T = T;
    option.N = N;
    option.D = D;
    option.Strike = K;
    option.lambda = lambda;
    option.optionType = "basket";
    option.fdStep = fdStep;

    MonteCarlo mc = {
        {D, r, rho, sigma, S0},
        option,
        M,
        pnl_rng_create(PNL_RNG_MERSENNE)
    };

    pnl_rng_sseed(mc.rng, 42);

    PnlMat* market =
        pnl_mat_create_from_file(
            "../data/basket_5d_1_market.txt"
        );

    if (market == nullptr) {
        std::cerr
            << "Impossible de lire le fichier market"
            << std::endl;

        pnl_rng_free(&mc.rng);
        pnl_vect_free(&sigma);
        pnl_vect_free(&S0);
        pnl_vect_free(&lambda);

        return 1;
    }

    PnlVect* row = pnl_vect_create(D);

    bool allSuccess = true;

    std::cout << std::setprecision(10);

    for (std::size_t dateIndex = 0;
         dateIndex < dates.size();
         ++dateIndex) {

        const int date = dates[dateIndex];

        const double t =
            static_cast<double>(date) * T / H;

        // N = 1 et t > 0 :
        // past contient S_0 puis S_t.
        PnlMat* past = pnl_mat_create(2, D);

        pnl_mat_get_row(row, market, 0);
        pnl_mat_set_row(past, row, 0);

        pnl_mat_get_row(row, market, date);
        pnl_mat_set_row(past, row, 1);

        const std::tuple<PnlVect*, PnlVect*>
            deltaResult = mc.delta(past, t);

        PnlVect* obtainedDeltas =
            std::get<0>(deltaResult);

        PnlVect* obtainedStdDevs =
            std::get<1>(deltaResult);

        bool dateSuccess = true;

        std::cout << "Date " << date << '\n';

        for (int asset = 0; asset < D; ++asset) {
            const double obtainedDelta =
                GET(obtainedDeltas, asset);

            const double obtainedStdDev =
                GET(obtainedStdDevs, asset);

            const double expectedDelta =
                expectedDeltas[dateIndex][asset];

            const double expectedStdDev =
                expectedDeltaStdDevs[dateIndex][asset];

            const double obtainedLower =
                obtainedDelta
                - confidenceFactor * obtainedStdDev;

            const double obtainedUpper =
                obtainedDelta
                + confidenceFactor * obtainedStdDev;

            const double expectedLower =
                expectedDelta
                - confidenceFactor * expectedStdDev;

            const double expectedUpper =
                expectedDelta
                + confidenceFactor * expectedStdDev;

            const bool intervalsIntersect =
                std::max(obtainedLower, expectedLower)
                <= std::min(
                    obtainedUpper,
                    expectedUpper
                );

            std::cout
                << "  Actif " << asset << '\n'
                << "    Delta obtenu  : "
                << obtainedDelta << '\n'
                << "    IC obtenu     : ["
                << obtainedLower << ", "
                << obtainedUpper << "]\n"
                << "    Delta attendu : "
                << expectedDelta << '\n'
                << "    IC attendu    : ["
                << expectedLower << ", "
                << expectedUpper << "]\n"
                << "    Résultat      : "
                << (
                    intervalsIntersect
                        ? "SUCCESS"
                        : "FAILED"
                )
                << '\n';

            dateSuccess =
                dateSuccess && intervalsIntersect;
        }

        std::cout
            << "  Date " << date << " : "
            << (dateSuccess ? "SUCCESS" : "FAILED")
            << "\n\n";

        allSuccess =
            allSuccess && dateSuccess;

        pnl_vect_free(&obtainedDeltas);
        pnl_vect_free(&obtainedStdDevs);
        pnl_mat_free(&past);
    }

    pnl_vect_free(&row);
    pnl_mat_free(&market);
    pnl_rng_free(&mc.rng);
    pnl_vect_free(&sigma);
    pnl_vect_free(&S0);
    pnl_vect_free(&lambda);

    return allSuccess ? 0 : 1;
}