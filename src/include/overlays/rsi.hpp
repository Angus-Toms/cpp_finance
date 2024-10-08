#pragma once 

#ifndef RSI_HPP
#define RSI_HPP

#include "ioverlay.hpp"

class PriceSeries;

class RSI : public IOverlay {
private:
    int period;
    std::map<std::time_t, double> data;

public:
    RSI(std::shared_ptr<PriceSeries> priceSeries, int period = 14);

    void checkArguments() override;
    void calculate() override;
    void plot() const override;
    TimeSeries<std::vector<double>> getDataMap() const override;
    std::vector<std::vector<std::string>> getTableData() const override;
};

#endif // RSI_HPP