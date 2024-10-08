#pragma once

#ifndef PRICESERIES_HPP
#define PRICESERIES_HPP

#include <Python.h>
#include <datetime.h>

#include <fstream>
#include <string>
#include <sstream>
#include <thread>
#include <map>
#include <memory>

#include "types.hpp"
#include "time_utils.hpp"
#include "print_utils.hpp"
#include "timeseries/timeseries_models.hpp"

#include "../../third_party/matplotlibcpp.h"

// Forward declaration of overlays 
class IOverlay;
class SMA;
class EMA;
class MACD;
class BollingerBands;
class RSI;

class PriceSeries {
private:
    std::string ticker;
    std::time_t start;
    std::time_t end;
    std::string interval;
    int count;

    std::vector<std::time_t> dates;
    std::vector<double> opens;
    std::vector<double> highs;
    std::vector<double> lows;
    std::vector<double> closes;
    std::vector<double> adjCloses;
    std::vector<long> volumes;

    std::vector<std::shared_ptr<IOverlay>> overlays;
    bool includeRSI = false;
    bool includeMACD = false;

    // Private constructor
    PriceSeries(const std::string& ticker, const std::time_t start, const std::time_t end, const std::string& interval);

    void checkArguments();
    void fetchData();

public:
    PriceSeries();
    ~PriceSeries();
    
    // static PyObject* fetchData(PyObject* self, PyObject* args);

    void plot(const std::string& type = "line", const bool includeVolume = false, const std::string& savePath = "") const;
    std::vector<std::vector<std::string>> getTableData() const;
    std::string toString(bool includeOverlays = false, bool changeHighlighting = true) const;

    // Factory methods ---------------------------------------------------------
    static std::unique_ptr<PriceSeries> getPriceSeries(const std::string& ticker, const std::time_t start, const std::time_t end, const std::string& interval);
    static std::unique_ptr<PriceSeries> getPriceSeries(const std::string& ticker, const std::string& start, const std::string& end, const std::string& interval);
    static std::unique_ptr<PriceSeries> getPriceSeries(const std::string& ticker, const std::time_t start, const std::time_t end);
    static std::unique_ptr<PriceSeries> getPriceSeries(const std::string& ticker, const std::string& start, const std::string& end);
    static std::unique_ptr<PriceSeries> getPriceSeries(const std::string& ticker, const std::time_t start, const std::string& interval, const std::size_t count);
    static std::unique_ptr<PriceSeries> getPriceSeries(const std::string& ticker, const std::string& start, const std::string& interval, const std::size_t count);

    // Getters -----------------------------------------------------------------
    int getCount() const;
    const std::string getTicker() const;
    const std::vector<std::time_t> getDates() const;
    const std::vector<double> getOpens() const;
    const std::vector<double> getHighs() const;
    const std::vector<double> getLows() const;
    const std::vector<double> getCloses() const;
    const std::vector<double> getAdjCloses() const;
    const std::vector<long> getVolumes() const;

    // Overlays ----------------------------------------------------------------
    void addOverlay(const std::shared_ptr<IOverlay> overlay);
    const std::vector<std::shared_ptr<IOverlay>>& getOverlays() const;

    void addSMA(int period = 20);
    void addEMA(int period = 20, double smoothingFactor = -1);
    void addMACD(int aPeriod = 12, int bPeriod = 26, int cPeriod = 9);
    void addBollingerBands(int period = 20, double numStdDev = 2, MovingAverageType maType = MovingAverageType::SMA);
    void addRSI(int period = 14);

    const std::shared_ptr<SMA> getSMA(int period = 20) const;
    const std::shared_ptr<EMA> getEMA(int period = 20, double smoothingFactor = -1) const;
    const std::shared_ptr<MACD> getMACD(int aPeriod = 12, int bPeriod = 26, int cPeriod = 9) const;
    const std::shared_ptr<BollingerBands> getBollingerBands(int period = 20, double numStdDev = 2, MovingAverageType maType = MovingAverageType::SMA) const;
    const std::shared_ptr<RSI> getRSI(int period = 14) const;

    // Time Series Analyses ----------------------------------------------------
    const std::shared_ptr<AR> getAR(int arOrder) const;
    const std::shared_ptr<MA> getMA(int maOrder) const;
    const std::shared_ptr<ARMA> getARMA(int arOrder, int maOrder) const;

    // Exports -----------------------------------------------------------------
    void exportCSV(const std::string& filename = "", const char delimiter = ',', const bool includeOverlays = true) const;

    // Testing setters 
    void setCloses(const std::vector<double>& closes);
    void setDates(const std::vector<std::time_t>& dates);
    void setCount(const int count);
};
#endif // PRICESERIES_HPP