#ifndef CHART_MANAGER_H
#define CHART_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <system/Logger.h>

enum NoteType { TAP, HOLD_START, HOLD_END, MINE };

struct NoteStruct {
    float time;
    int column;
    NoteType type;
    
    bool operator<(const NoteStruct& other) const {
        return time < other.time;
    }
};

struct TimingPoint {
    float time;
    double bpm;
};

struct ChartData {
    std::string filename;
    std::string filePath;

    std::vector<NoteStruct> notes;
    std::vector<TimingPoint> timingPoints;
    std::map<std::string, std::string> metadata;
    int keyCount = 4;
};

class ChartUtils {
public:
    static std::string getChartBackgroundName(const ChartData& chartData);
    static std::string getChartInfo(const ChartData& chartData, float selectedRate);

    static ChartData parseChart(const std::string& filePath, const std::string& filename, const std::string& content);
    static std::map<std::string, ChartData> parseChartMultiple(const std::string& filePath, const std::string& filename, const std::string& content);
    static std::string saveVsc(const std::string& filename, const ChartData& chartData);
    
private:
    static ChartData parseVsc(const std::string& content);
    static ChartData convertOsuToChartData(const std::string& osuContent);
    static std::vector<ChartData> convertSmToChartDataMultiple(const std::string& smContent);
};

#endif