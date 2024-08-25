#include "priceseries.hpp"

#include "overlays/ioverlay.hpp"
#include "overlays/bollinger.hpp"
#include "overlays/ema.hpp"
#include "overlays/macd.hpp"
#include "overlays/rsi.hpp"
#include "overlays/sma.hpp"

PriceSeries::PriceSeries() = default;
PriceSeries::~PriceSeries() = default;
PriceSeries::PriceSeries(const std::string& ticker, const std::time_t start, const std::time_t end, const std::string& interval)
    : ticker(ticker), start(start), end(end), interval(interval) {
    checkArguments();
    fetchCSV();
}

void PriceSeries::checkArguments() {
    // TODO: Validate ticker

    // Validate start time 
    if (start < 0) {
        throw std::invalid_argument("Could not get PriceSeries. Invalid start time");
    }

    if (start > std::time(nullptr)) {
        throw std::invalid_argument("Could not get PriceSeries. Start time is in the future");
    }

    if (start > end) {
        throw std::invalid_argument("Could not get PriceSeries. Start time is after end time");
    }

    // Validate end time
    if (end > std::time(nullptr)) {
        std::cout << "WARNING! End time is in future, cropping to current time\n";
        end = std::time_t(nullptr);
    }

    // Valid interval check
    if (isInvalidInterval(interval)) {
        std::ostringstream supportedIntervals;
        for (const auto& interval : VALID_INTERVALS) {
            supportedIntervals << interval << " ";
        }
        throw std::invalid_argument("Could not get PriceSeries. Interval " + interval + " is not supported\nSupported intervals: " + supportedIntervals.str());
    }
}

void PriceSeries::fetchCSV() {
    // Call construction
    std::ostringstream urlBuilder;
    urlBuilder << "https://query1.finance.yahoo.com/v7/finance/download/" << ticker
                << "?period1=" << start
                << "&period2=" << end
                << "&interval=" << interval;
    std::string url = urlBuilder.str(); // TODO: Use fmt here

    CURL* curl = curl_easy_init();
    std::string readBuffer;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallBack);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "Failed to fetch " << ticker << ": " << curl_easy_strerror(res) << std::endl;
        }
        curl_easy_cleanup(curl);
    }
    parseCSV(readBuffer);
}

void PriceSeries::parseCSV(const std::string& readBuffer) {
    std::istringstream ss(readBuffer);
    std::string line;

    bool isFirstLine = true;
    while (std::getline(ss, line)) {
        // Skip header line
        if (isFirstLine) {
            isFirstLine = false;
            continue;
        }

        std::istringstream lineStream(line);
        std::string dateStr, openStr, highStr, lowStr, closeStr, adjCloseStr, volumeStr;
        std::getline(lineStream, dateStr, ',');
        dates.push_back(dateStringToEpoch(dateStr));
        std::getline(lineStream, openStr, ',');
        opens.push_back(std::stod(openStr));
        std::getline(lineStream, highStr, ',');
        highs.push_back(std::stod(highStr));
        std::getline(lineStream, lowStr, ',');
        lows.push_back(std::stod(lowStr));
        std::getline(lineStream, closeStr, ',');
        closes.push_back(std::stod(closeStr));
        std::getline(lineStream, adjCloseStr, ',');
        adjCloses.push_back(std::stod(adjCloseStr));
        std::getline(lineStream, volumeStr, ',');
        volumes.push_back(std::stol(volumeStr));
        count++;
    }
}

std::tuple<std::vector<std::time_t>, std::vector<std::string>> getTicks(std::time_t start,
                                                                        std::time_t end,
                                                                        int nTicks) {
    std::vector<std::time_t> ticks;
    std::vector<std::string> labels; 

    std::time_t interval = (end - start) / (nTicks-1);
    for (int i = 0; i < nTicks; ++i) {
        ticks.push_back(start + i*interval);
        labels.push_back(epochToDateString(start + i*interval));
    }
    return std::make_tuple(ticks, labels);
}

void plotLine(const std::vector<std::time_t>& xs, const std::vector<double>& ys) {
    namespace plt = matplotlibcpp;
    plt::named_plot("Price", xs, ys);
}

void plotCandleStick(const std::vector<std::time_t>& xs,
                     const std::vector<double>& opens,
                     const std::vector<double>& highs,
                     const std::vector<double>& lows,
                     const std::vector<double>& closes,
                     double width) {
    namespace plt = matplotlibcpp;
    std::vector<double> tops, bottoms, topWicks;
    std::vector<std::string> colors;

    // Find bullish and bearish days
    for (size_t i = 0; i < xs.size(); ++i) {
        const auto close = closes[i];
        const auto open = opens[i];
        topWicks.push_back(highs[i] - lows[i]);
        if (close > open) {
            tops.push_back(close-open);
            bottoms.push_back(open);
            colors.push_back("green");
        } else {
            tops.push_back(open-close);
            bottoms.push_back(close);
            colors.push_back("red");
        }
    }

    // Plot bars
    plt::bar(xs, tops, bottoms, width, 0, colors);
    // Plot wicks 
    plt::bar(xs, topWicks, lows, width/8, 0, colors);
}

void plotArea(const std::vector<std::time_t>& xs, const std::vector<double>& ys) {
    namespace plt = matplotlibcpp;
    std::vector<double> zeros(ys.size(), 0.0);
    std::map<std::string, std::string> kwargs = {
        {"color", "darkblue"}
    };
    plt::fill_between(xs, ys, zeros, kwargs, 0.2, 0);
    plt::ylim(*std::min_element(ys.begin(), ys.end()) * 0.95, *std::max_element(ys.begin(), ys.end()) * 1.05);
}

void PriceSeries::plot(const std::string& type, const bool includeVolume) {
    // Plot assumptions, only plot a single RSI and MACD subplot
    // Each subplot is given 1/5 of the height
    namespace plt = matplotlibcpp;
    const auto& [ticks, labels] = getTicks(dates.front(), dates.back(), 6);
    int priceHeight = 5 - includeVolume - includeRSI - includeMACD;

    // Make main price plot
    plt::figure_size(1200, 800);
    plt::subplot2grid(5, 1, 0, 0, priceHeight, 1);
    plt::ylabel("Price ($)");
    if (type == "line") {
        plotLine(dates, closes);
    } else if (type == "candlestick") {
        plotCandleStick(dates, opens, highs, lows, closes, intervalToSeconds("1d")*0.8);
    } else if (type == "area") {
        plotArea(dates, closes);
    }
    plt::title(ticker);
    plt::grid(true);
    plt::xlim(dates.front() - intervalToSeconds("1d"), dates.back() + intervalToSeconds("1d"));

    if (priceHeight == 5) {
        plt::xticks(ticks, labels);
    } else {
        plt::xticks(std::vector<double>(), std::vector<std::string>());
    }

    // Plot non-RSI and non-MACD overlays 
    for (const auto& overlay : overlays) {
        if (overlay->getName().find("RSI") != 0 && overlay->getName().find("MACD") != 0) {
            overlay->plot();
            plt::legend();
        }
    }

    if (includeVolume) {
        plt::subplot2grid(5, 1, priceHeight, 0, 1, 1);
        plt::bar(dates, volumes, {}, intervalToSeconds("1d")*0.8, 0);
        plt::xlim(dates.front() - intervalToSeconds("1d"), dates.back() + intervalToSeconds("1d"));
        plt::ylabel("Volume");
        priceHeight++;
        if (priceHeight == 5) {
            plt::xticks(ticks, labels);
        } else {
            plt::xticks(std::vector<double>(), std::vector<std::string>());
        }
    }
    
    // Plot RSI and MACD overlays
    if (includeRSI) {
        plt::subplot2grid(5, 1, priceHeight, 0, 1, 1);
        for (const auto& overlay : overlays) {
            if (overlay->getName().find("RSI") == 0) {
                overlay->plot();
                break;
            }
        }
        priceHeight++;
        if (priceHeight == 5) {
            plt::xticks(ticks, labels);
        } else {
            plt::xticks(std::vector<double>(), std::vector<std::string>());
        }

    }
    if (includeMACD) {
        plt::subplot2grid(5, 1, priceHeight, 0, 1, 1);
        for (const auto& overlay : overlays) {
            if (overlay->getName().find("MACD") == 0) {
                overlay->plot();
                break;
            }
        }
        if (priceHeight == 4) {
            plt::xticks(ticks, labels);
        } else {
            plt::xticks(std::vector<double>(), std::vector<std::string>());
        }
    }
    plt::tight_layout();
    plt::show();
}

std::vector<std::vector<std::string>> PriceSeries::getTableData() const {
    std::vector<std::vector<std::string>> tableData;
    for (size_t i = 0; i < dates.size(); ++i) {
        tableData.push_back({
            epochToDateString(dates[i]),
            fmt::format("{}", opens[i]),
            fmt::format("{}", highs[i]),
            fmt::format("{}", lows[i]),
            fmt::format("{}", closes[i]),
            fmt::format("{}", adjCloses[i]),
            fmt::format("{}", volumes[i])
        });
    }
    return tableData;
}

std::string PriceSeries::toString(bool includeOverlays, bool changeHighlighting) const {
    std::vector<int> columnWidths = {12, 10, 10, 10, 10, 12, 12};
    std::vector<std::string> columnHeaders = {"Date", "Open", "High", "Low", "Close", "adjClose", "Volume"};
    std::vector<std::vector<std::string>> tableData = getTableData();

    if (includeOverlays) {
        for (const auto& overlay : overlays) {
            const auto& overlayData = overlay->getDataMap();
            // Find size of value vectors 
            std::size_t n = overlayData.begin()->second.size();

            // Add column headers and widths
            const auto& newHeaders = overlay->getColumnHeaders();
            const auto& newWidths = overlay->getColumnWidths();
            for (size_t i = 0; i < n; ++i) {
                columnHeaders.push_back(newHeaders[i+1]);
                columnWidths.push_back(newWidths[i+1]);
            }

            size_t dateCount = dates.size();
            for (size_t i = 0; i < dateCount; ++i) {
                const auto& date = dates[i];
                // Check if a corresponding entry exists in the overlay data map
                if (overlayData.find(date) != overlayData.end()) {
                    // Add n overlay datapoints to row
                    for (const auto& overlayVal : overlayData.at(date)) {
                        tableData.at(i).push_back(fmt::format("{:.2f}", overlayVal));
                    }
                } else {
                    // If not, add n blank datapoints to row
                    for (size_t j = 0; j < n; ++j) {
                        tableData.at(i).push_back(" ");
                    }
                }
            }
        }
    }
    // TODO: write return
    return getTable(ticker, tableData, columnWidths, columnHeaders, changeHighlighting);
}

// Factory methods -------------------------------------------------------------
std::unique_ptr<PriceSeries> PriceSeries::getPriceSeries(const std::string& ticker, const std::time_t start, const std::time_t end, const std::string& interval) {
    return std::unique_ptr<PriceSeries>(new PriceSeries(ticker, start, end, interval));
}

std::unique_ptr<PriceSeries> PriceSeries::getPriceSeries(const std::string& ticker, const std::string& start, const std::string& end, const std::string& interval) {
    return getPriceSeries(ticker, dateStringToEpoch(start), dateStringToEpoch(end), interval);
}

std::unique_ptr<PriceSeries> PriceSeries::getPriceSeries(const std::string& ticker, const std::time_t start, const std::time_t end) {
    return getPriceSeries(ticker, start, end, "1d");
}

std::unique_ptr<PriceSeries> PriceSeries::getPriceSeries(const std::string& ticker, const std::string& start, const std::string& end) {
    return getPriceSeries(ticker, dateStringToEpoch(start), dateStringToEpoch(end), "1d");
}

std::unique_ptr<PriceSeries> PriceSeries::getPriceSeries(const std::string& ticker, const std::time_t start, const std::string& interval, const std::size_t count) {
    std::time_t end = start + count * intervalToSeconds(interval);
    return getPriceSeries(ticker, start, end, interval);
}

std::unique_ptr<PriceSeries> PriceSeries::getPriceSeries(const std::string& ticker, const std::string& start, const std::string& interval, const std::size_t count) {
    std::time_t startTime = dateStringToEpoch(start);
    std::time_t end = startTime + count * intervalToSeconds(interval);
    return getPriceSeries(ticker, startTime, end, interval);
}

// Getters ---------------------------------------------------------------------
int PriceSeries::getCount() const { return count; }
const std::string PriceSeries::getTicker() const { return ticker; }
const std::vector<std::time_t> PriceSeries::getDates() const { return dates; }
const std::vector<double> PriceSeries::getOpens() const { return opens; }
const std::vector<double> PriceSeries::getHighs() const { return highs;}
const std::vector<double> PriceSeries::getLows() const { return lows; }
const std::vector<double> PriceSeries::getCloses() const { return closes; }
const std::vector<double> PriceSeries::getAdjCloses() const { return adjCloses; }
const std::vector<long> PriceSeries::getVolumes() const { return volumes;}

// Overlays --------------------------------------------------------------------
void PriceSeries::addOverlay(const std::shared_ptr<IOverlay> overlay) {
    overlays.push_back(std::move(overlay));
}

const std::vector<std::shared_ptr<IOverlay>>& PriceSeries::getOverlays() const {
    return overlays;
}

void PriceSeries::addSMA(int period) {
    addOverlay(getSMA(period));
}

void PriceSeries::addEMA(int period, double smoothingFactor) {
    addOverlay(getEMA(period, smoothingFactor));
}

void PriceSeries::addMACD(int aPeriod, int bPeriod, int cPeriod) {
    addOverlay(getMACD(aPeriod, bPeriod, cPeriod));
    includeMACD = true;
}

void PriceSeries::addBollingerBands(int period, double numStdDev, MovingAverageType maType) {
    addOverlay(getBollingerBands(period, numStdDev, maType));
}

void PriceSeries::addRSI(int period) {
    addOverlay(getRSI(period));
    includeRSI = true;
}

const std::shared_ptr<SMA> PriceSeries::getSMA(int period) const {
    try {
        return std::make_shared<SMA>(std::make_shared<PriceSeries>(*this), period);
    } catch (const std::invalid_argument& e) {
        std::cerr << "Could not construct SMA: " << e.what() << std::endl;
        return nullptr;
    }
}

const std::shared_ptr<EMA> PriceSeries::getEMA(int period, double smoothingFactor) const {
    try {
        return std::make_shared<EMA>(std::make_shared<PriceSeries>(*this), period, smoothingFactor);
    } catch (const std::invalid_argument& e) {
        std::cerr << "Could not construct EMA: " << e.what() << std::endl;
        return nullptr;
    }
}

const std::shared_ptr<MACD> PriceSeries::getMACD(int aPeriod, int bPeriod, int cPeriod) const {
    try {
        return std::make_shared<MACD>(std::make_shared<PriceSeries>(*this), aPeriod, bPeriod, cPeriod);
    } catch (const std::invalid_argument& e) {
        std::cerr << "Could not construct MACD: " << e.what() << std::endl;
        return nullptr;
    }
}

const std::shared_ptr<BollingerBands> PriceSeries::getBollingerBands(int period, double numStdDev, MovingAverageType maType) const {
    try {
        return std::make_shared<BollingerBands>(std::make_shared<PriceSeries>(*this), period, numStdDev, maType);
    } catch (const std::invalid_argument& e) {
        std::cerr << "Could not construct BollingerBands: " << e.what() << std::endl;
        return nullptr;
    }
}

const std::shared_ptr<RSI> PriceSeries::getRSI(int period) const {
    try {
        return std::make_shared<RSI>(std::make_shared<PriceSeries>(*this), period);
    } catch (const std::invalid_argument& e) {
        std::cerr << "Could not construct RSI: " << e.what() << std::endl;
        return nullptr;
    }
}

// Exports ---------------------------------------------------------------------
void PriceSeries::exportToCSV(const std::string& filename, 
                              const char delimiter, 
                              const bool includeOverlays) const {
    std::string path = filename == "" ? fmt::format("{}.csv", ticker) : filename;

    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "Could not open file " << path << std::endl;
        return;
    }

    std::vector<std::vector<std::string>> tableData = getTableData();
    if (includeOverlays) {
        for (const auto& overlay : overlays) {
            const auto& overlayData = overlay->getDataMap();
            // Find size of value vectors 
            std::size_t n = overlayData.begin()->second.size();

            size_t dateCount = dates.size();
            for (size_t i = 0; i < dateCount; ++i) {
                const auto& date = dates[i];
                // Check if corresponding entry exists in the overlay data map
                if (overlayData.find(date) != overlayData.end()) {
                    // Add n overlay datapoints to row
                    for (const auto& overlayVal : overlayData.at(date)) {
                        tableData.at(i).push_back(fmt::format("{:.2f}", overlayVal));
                    }
                } else {
                    // If not, add n blank datapoints to row
                    for (size_t j = 0; j < n; ++j) {
                        tableData.at(i).push_back("");
                    }
                }
            }
        }
    }
    // Write to file 
    for (const auto& row : tableData) {
        for (size_t i = 0; i < row.size(); ++i) {
            file << row[i];
            if (i < row.size() - 1) {
                file << delimiter;
            }
        }
        file << "\n";
    }
}