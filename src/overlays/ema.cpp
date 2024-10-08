#include "overlays/ema.hpp"
#include "priceseries.hpp"

EMA::EMA(std::shared_ptr<PriceSeries> priceSeries, int period, double smoothingFactor)
    : IOverlay(priceSeries), period(period), smoothingFactor(smoothingFactor) {

    // Calculate smoothing factor if not specified
    if (this->smoothingFactor == -1) {
        this->smoothingFactor = 2.0 / (period+1);
    }

    // Set table printing values
    name = fmt::format("EMA({}d, α={:.2f})", period, this->smoothingFactor);
    columnHeaders = {"Date", "EMA"};
    columnWidths = {12, 10};

    checkArguments();
    calculate();
}

void EMA::checkArguments() {
    if (period < 1) {
        throw std::invalid_argument("Could not construct EMA: period must be greater than 0");
    }
    if (period > priceSeries->getCount()) {
        throw std::invalid_argument("Could not construct EMA: period must be less than the number of data points");
    }
    if (smoothingFactor < 0 || smoothingFactor > 1) {
        std::cout << smoothingFactor << "\n";
        throw std::invalid_argument("Could not construct EMA: smoothing factor must be between 0 and 1");
    }
}

void EMA::calculate() {
    const std::vector<std::time_t>& dates = priceSeries->getDates();
    const std::vector<double> closes = priceSeries->getCloses();

    // Calculate SMA of first window
    double ema = 0.0;
    for (int i = 0; i < period; i++) {
        ema += closes[i];
    }
    ema /= period;

    // Slide window until end of data
    data[dates[period-1]] = ema;
    for (size_t i = period; i < closes.size(); i++) {
        ema = (closes[i] * smoothingFactor) + (ema * (1 - smoothingFactor));
        data[dates[i]] = ema;
    }
}

void EMA::plot() const {
    namespace plt = matplotlibcpp;

    std::vector<std::time_t> xs;
    std::vector<double> ys;
    for (const auto& [date, ema] : data) {
        xs.push_back(date);
        ys.push_back(ema);
    }

    plt::named_plot(name, xs, ys);   
}

std::vector<std::vector<std::string>> EMA::getTableData() const {
    std::vector<std::vector<std::string>> tableData;
    for (const auto& [date, ema] : data) {
        tableData.push_back({
            fmt::format(epochToDateString(date)),
            fmt::format("{:.2f}", ema)
        });
    }
    return tableData;
}

TimeSeries<std::vector<double>> EMA::getDataMap() const {
    TimeSeries<std::vector<double>> dataMap;
    for (const auto& [date, ema] : data) {
        dataMap[date] = {ema};
    }
    return dataMap;
}

const TimeSeries<double> EMA::getData() const {
    return data;
}