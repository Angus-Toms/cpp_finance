#pragma once 

#ifndef SMA_HPP
#define SMA_HPP

#include "ioverlay.hpp"

class PriceSeries;

class SMA : public IOverlay {
private:
    int period;
    TimeSeries<double> data;

public:
    SMA(std::shared_ptr<PriceSeries> priceSeries, int period = 20);

    void checkArguments() override;
    void calculate() override;
    void plot() const override;
    TimeSeries<std::vector<double>> getDataMap() const override;
    std::vector<std::vector<std::string>> getTableData() const override;

    const TimeSeries<double> getData() const;
};

#endif // SMA_HPP