#include <rhythm/ChartManager.h>
#include <utils/Utils.h>
#include <sstream>
#include <cmath>
#include <iostream>

ChartData ChartManager::parseVsc(const std::string& content) {
    ChartData data;

    std::stringstream ss(content);
    std::string line, currentSection;

    while (std::getline(ss, line)) {
        line = Utils::trim(line);
        if (line.empty() || line.rfind("//", 0) == 0) continue;

        if (line.front() == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.size() - 2);
        } else {
            if (currentSection == "VSC") {
                size_t eqPos = line.find('=');
                if (eqPos != std::string::npos) {
                    data.metadata[line.substr(0, eqPos)] = line.substr(eqPos + 1);
                }
            } else if (currentSection == "TIMING") {
                std::stringstream lss(line);
                float time; std::string tag; double bpm;
                lss >> time >> tag >> bpm;
                if (tag == "BPM") data.timingPoints.push_back({time, bpm});
            } else if (currentSection == "NOTES") {
                std::stringstream lss(line);
                float time; int col; std::string typeStr;
                lss >> time >> col >> typeStr;
                
                NoteType type = TAP;
                if (typeStr == "HOLD_START") type = HOLD_START;
                else if (typeStr == "HOLD_END") type = HOLD_END;
                
                data.notes.push_back({time, col, type});
            }
        }
    }
    
    if (data.metadata.count("keys")) {
        try {
            data.keyCount = std::stoi(data.metadata["keys"]);
        } catch (...) {
            data.keyCount = 4;
        }
    }
    
    std::sort(data.notes.begin(), data.notes.end());
    return data;
}

ChartData ChartManager::convertOsuToChartData(const std::string& osuContent) {
    ChartData data;
    std::stringstream ss(osuContent);
    std::string line;
    std::string section = "";

    while (std::getline(ss, line)) {
        std::string trimmedLine = Utils::trim(line);
        if (trimmedLine.empty() || trimmedLine.rfind("//", 0) == 0) continue;

        if (trimmedLine.front() == '[' && trimmedLine.back() == ']') {
            section = trimmedLine.substr(1, trimmedLine.size() - 2);
            continue;
        }

        if (section == "Metadata") {
            size_t colonPos = trimmedLine.find(':');
            if (colonPos != std::string::npos) {
                std::string key = Utils::trim(trimmedLine.substr(0, colonPos));
                std::string val = Utils::trim(trimmedLine.substr(colonPos + 1));
                std::transform(key.begin(), key.end(), key.begin(), ::tolower);
                
                if (key == "title") data.metadata["title"] = val;
                else if (key == "artist") data.metadata["artist"] = val;
                else if (key == "creator") data.metadata["charter"] = val;
                else if (key == "version") data.metadata["difficulty"] = val;
                else if (key == "audio") data.metadata["audio"] = val;
            }
        }
        else if (section == "Difficulty") {
            if (trimmedLine.rfind("CircleSize:", 0) == 0) {
                size_t colonPos = trimmedLine.find(':');
                data.keyCount = std::stoi(Utils::trim(trimmedLine.substr(colonPos + 1)));
                data.metadata["keys"] = std::to_string(data.keyCount);
            }
        }
        else if (section == "TimingPoints") {
            auto parts = Utils::split(trimmedLine, ',');
            if (parts.size() >= 7 && std::stoi(parts[6]) == 1) { 
                float time = std::floor(std::stod(parts[0]));
                double msPerBeat = std::stod(parts[1]);
                if (msPerBeat > 0) {
                    double bpm = 60000.0 / msPerBeat;
                    data.timingPoints.push_back({time, bpm});
                }
            }
        }
        else if (section == "HitObjects") {
            auto parts = Utils::split(trimmedLine, ',');
            if (parts.size() >= 5) {
                int x = std::stoi(parts[0]);
                float time = std::stof(parts[2]);
                int type = std::stoi(parts[3]);
                
                int column = std::min(static_cast<int>(std::floor(x * data.keyCount / 512.0)), data.keyCount - 1);
                
                bool isHold = (type & 128) > 0;

                if (isHold && parts.size() >= 6) {
                    std::string objectParams = parts[5];
                    size_t colonPos = objectParams.find(':');
                    float endTime = (colonPos != std::string::npos) 
                                ? std::stoi(objectParams.substr(0, colonPos)) 
                                : std::stoi(objectParams);

                    if (endTime > time) {
                        data.notes.push_back({time, column, HOLD_START});
                        data.notes.push_back({endTime, column, HOLD_END});
                    }
                } 
                else {
                    data.notes.push_back({time, column, TAP});
                }
            }
        }
    }

    std::sort(data.notes.begin(), data.notes.end());
    return data;
}

ChartData ChartManager::parseChart(const std::string& filename, const std::string& content) {
    ChartData data;
    if (Utils::hasEnding(filename, ".osu")) {
        data = convertOsuToChartData(content);
    }  else {
        data = parseVsc(content);
    }
    data.filename = filename;
    return data;
}