#pragma once 

#ifndef TIMESERIES_MODELS_HPP
#define TIMESERIES_MODELs_HPP

#include "../types.hpp"
#include "../time_utils.hpp"
#include "../print_utils.hpp"

#include <vector>
#include <iostream>
#include <cmath>
#include <numeric>

#include "../../third_party/Eigen/Dense"
#include <nlopt.hpp>
#include "../../third_party/matplotlibcpp.h"

class TimeSeriesModel {
protected:
    double mse;
    double rmse;
    double mae;

    double c;

    std::string name;
    TimeSeries<double> data;
    TimeSeries<double> forecasted;
    size_t count;

public:
    TimeSeriesModel() = default;
    virtual ~TimeSeriesModel() = default;

    virtual void forecast(int steps) = 0;
    // virtual TimeSeries<double> forecast(std::time_t start) = 0; // TODO: Forecast until requested time

    TimeSeries<double> getForecasted() const {
        return forecasted;
    }

    int plot() const {
        namespace plt = matplotlibcpp;

        std::vector<std::time_t> dataXs;
        std::vector<double> dataYs;
        for (const auto& [date, value] : data) {
            dataXs.push_back(date);
            dataYs.push_back(value);
        }

        std::vector<std::time_t> forecastedXs;
        forecastedXs.push_back(data.rbegin()->first);
        std::vector<double> forecastedYs;
        forecastedYs.push_back(data.rbegin()->second);
        for (const auto& [date, value] : forecasted) {
            forecastedXs.push_back(date);
            forecastedYs.push_back(value);
        }

        plt::named_plot("Historical Data", dataXs, dataYs, "b");
        plt::named_plot("Forecasted", forecastedXs, forecastedYs, "r");

        plt::xlabel("Date");
        plt::ylabel("Price ($)");
        plt::title(name);

        const auto& [ticks, labels] = forecastedXs.size() == 0 ? 
            getTicks(dataXs.front(), dataXs.back(), 6) : 
            getTicks(dataXs.front(), forecastedXs.back(), 6);
        plt::xticks(ticks, labels);

        plt::tight_layout();
        plt::legend();
        plt::show();

        return 0;
    }

    virtual std::string toString() const = 0;
};

// TODO: Constructors for non-timeseries?
class AR : public TimeSeriesModel {
private:
    int arOrder;
    std::vector<double> phis; // AR coefficients

public:
    AR(const TimeSeries<double>& data);

    void train(int arOrder);
    void forecast(int steps) override;

    std::vector<double> getPhis() const;

    std::string toString() const override;
};

class MA : public TimeSeriesModel {
private:
    int maOrder;
    std::vector<double> thetas; // MA coefficients

public:
    MA(const TimeSeries<double>& data);

    void train(int maOrder);
    void forecast(int steps) override;

    std::vector<double> getThetas() const;

    std::string toString() const override;
};

class ARMA : public TimeSeriesModel {
private:
    int arOrder;
    int maOrder;
    std::vector<double> phis; // AR coefficients
    std::vector<double> thetas; // MA coefficients

public:
    ARMA(const TimeSeries<double>& data);

    void train(int arOrder, int maOrder);
    void forecast(int steps) override;

    std::vector<double> getPhis() const;
    std::vector<double> getThetas() const;

    std::string toString() const override;
};

#endif // TIMESERIES_MODELS_HPP