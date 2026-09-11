#include <iostream>
#include "json_helper.hpp"
#include "pricing_results.hpp"

PricingResults::PricingResults(double p_price, double p_priceStdDev, const PnlVect* const p_delta, const PnlVect* const p_deltaStdDev)
    : price(p_price)
    , priceStdDev(p_priceStdDev)
    , delta(p_delta)
    , deltaStdDev(p_deltaStdDev)
{ }


std::ostream& operator<<(std::ostream& o, const PricingResults& res)
{
    nlohmann::json j = {
        {"price", res.price},
        {"priceStdDev", res.priceStdDev},
        {"delta", const_cast<PnlVect*>(res.delta)},
        {"deltaStdDev", const_cast<PnlVect*>(res.deltaStdDev)}
    };
    o << std::setw(4) << j << std::endl;
    return o;
}