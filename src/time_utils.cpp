#include "time_utils.hpp"

std::string epochToDateString(const std::time_t date, bool includeTime) {
    std::tm* tm = std::localtime(&date);
    if (tm == nullptr) {
        return "Invalid time";
    }
    std::ostringstream oss;
    if (includeTime) {
        oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
    } else {
        oss << std::put_time(tm, "%Y-%m-%d");
    }
    return oss.str();
}

std::time_t dateStringToEpoch(const std::string& dateStr) {
    // TODO: Use fmt
    std::tm tm = {};
    std::istringstream ss(dateStr);

    ss >> std::get_time(&tm, "%Y-%m-%d");
    if (ss.fail()) {
        std::cerr << "Failed to parse date string " << dateStr << std::endl;
        return -1;
    }

    std::time_t time = std::mktime(&tm);
    if (time == -1) {
        std::cerr << "Error creating time_t object" << std::endl;
        return -1;
    }
    return time;
}

std::time_t intervalToSeconds(const std::string& interval) {
    if (interval == "1m") {
        return MINUTE_DURATION;
    } else if (interval == "1h") {
        return HOUR_DURATION;
    } else if (interval == "1d") {
        return DAY_DURATION;
    } else if (interval == "1wk") {
        return WEEK_DURATION;
    } else if (interval == "1mo") {
        return MONTH_DURATION;
    } else if (interval == "1y") {
        return YEAR_DURATION;
    } else {
        return -1;
    }
}

bool isInvalidInterval(const std::string& interval) {
    return std::find(VALID_INTERVALS.begin(), VALID_INTERVALS.end(), interval) == VALID_INTERVALS.end();
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