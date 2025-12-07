#include <utils/rhythm/ChartUtils.h>
#include <utils/Utils.h>
#include <sstream>
#include <cmath>
#include <iostream>

std::string ChartUtils::getChartBackgroundName(const ChartData& chartData) {
    auto it = chartData.metadata.find("background");
    if (it != chartData.metadata.end()) {
        return it->second;
    }
    return "";
}

std::string ChartUtils::getChartInfo(const ChartData& chartData, float selectedRate) {
    std::stringstream ss;

    auto itArtist = chartData.metadata.find("artist");
    auto itTitle = chartData.metadata.find("title");
    auto itDifficulty = chartData.metadata.find("difficulty");

    ss << (itArtist != chartData.metadata.end() ? itArtist->second : "Unknown") << " - ";
    ss << (itTitle != chartData.metadata.end() ? itTitle->second : "Unknown") << "\n[";
    ss << (itDifficulty != chartData.metadata.end() ? itDifficulty->second : "Unknown") << " " << selectedRate << "x]";
    
    return ss.str();
}

float calculateBeatsToSeconds(float targetBeat, float offset, const std::vector<TimingPoint> &timingPoints)
{
    if (timingPoints.empty())
    {
        double initialBPM = 120.0;
        return (targetBeat * 60.0f / (float)initialBPM) + -offset;
    }

    float currentTime = 0.0f;
    float currentBeat = 0.0f;
    double currentBPM = (timingPoints.front().time == 0.0f) ? timingPoints.front().bpm : 120.0;

    currentTime = -offset;

    if (targetBeat < timingPoints.front().time)
    {
        return (targetBeat * 60.0f / (float)currentBPM) + -offset;
    }

    for (size_t i = 0; i < timingPoints.size(); ++i)
    {
        const TimingPoint &tp = timingPoints[i];

        if (tp.time > currentBeat)
        {
            float segmentBeats = tp.time - currentBeat;
            currentTime += (segmentBeats * 60.0f / (float)currentBPM);
            currentBeat = tp.time;
        }

        currentBPM = tp.bpm;

        if (i + 1 < timingPoints.size())
        {
            const TimingPoint &nextTp = timingPoints[i + 1];

            if (targetBeat < nextTp.time)
            {
                float remainingBeats = targetBeat - currentBeat;
                currentTime += (remainingBeats * 60.0f / (float)currentBPM);
                return currentTime;
            }
        }
        else
        {
            float remainingBeats = targetBeat - currentBeat;
            if (remainingBeats >= 0.0f)
            {
                currentTime += (remainingBeats * 60.0f / (float)currentBPM);
            }
            return currentTime;
        }
    }

    return currentTime;
}

ChartData ChartUtils::parseVsc(const std::string &content)
{
    ChartData data;

    std::stringstream ss(content);
    std::string line, currentSection;

    while (std::getline(ss, line))
    {
        line = Utils::trim(line);
        if (line.empty() || line.rfind("//", 0) == 0)
            continue;

        if (line.front() == '[' && line.back() == ']')
        {
            currentSection = line.substr(1, line.size() - 2);
        }
        else
        {
            if (currentSection == "VSC")
            {
                size_t eqPos = line.find('=');
                if (eqPos != std::string::npos)
                {
                    data.metadata[line.substr(0, eqPos)] = line.substr(eqPos + 1);
                }
            }
            else if (currentSection == "TIMING")
            {
                std::stringstream lss(line);
                float time;
                std::string tag;
                double bpm;
                lss >> time >> tag >> bpm;
                if (tag == "BPM")
                    data.timingPoints.push_back({time, bpm});
            }
            else if (currentSection == "NOTES")
            {
                std::stringstream lss(line);
                float time;
                int col;
                std::string typeStr;
                lss >> time >> col >> typeStr;

                NoteType type = TAP;
                if (typeStr == "HOLD_START")
                    type = HOLD_START;
                else if (typeStr == "HOLD_END")
                    type = HOLD_END;

                data.notes.push_back({time, col, type});
            }
        }
    }

    if (data.metadata.count("keys"))
    {
        try
        {
            data.keyCount = std::stoi(data.metadata["keys"]);
        }
        catch (...)
        {
            data.keyCount = 4;
        }
    }

    std::sort(data.notes.begin(), data.notes.end());
    return data;
}

std::string ChartUtils::saveVsc(const std::string &filename, const ChartData &chartData)
{
    std::stringstream ss;
    ss << "[VSC]\n";
    for (const auto &kv : chartData.metadata)
    {
        ss << kv.first << "=" << kv.second << "\n";
    }
    ss << "\n[TIMING]\n";
    for (const auto &tp : chartData.timingPoints)
    {
        ss << tp.time << " BPM " << tp.bpm << "\n";
    }
    ss << "\n[NOTES]\n";
    for (const auto &note : chartData.notes)
    {
        std::string typeStr = "TAP";
        if (note.type == HOLD_START)
            typeStr = "HOLD_START";
        else if (note.type == HOLD_END)
            typeStr = "HOLD_END";

        ss << note.time << " " << note.column << " " << typeStr << "\n";
    }
    
    std::string vscContent = ss.str();

    size_t lastDot = filename.find_last_of('.');
    std::string baseFilename = (lastDot == std::string::npos) ? filename : filename.substr(0, lastDot);
    std::string vscFilePath = baseFilename + ".vsc";

    std::ofstream outFile(vscFilePath);
    if (outFile.is_open())
    {
        outFile << vscContent;
        outFile.close();

        GAME_LOG_DEBUG("ChartManager: Saved VSC file to " + vscFilePath);
    }
    else
    {
        GAME_LOG_ERROR("ChartManager: Could not open " + vscFilePath + " for writing.");
    }
    return vscContent;
}

ChartData ChartUtils::convertOsuToChartData(const std::string &osuContent)
{
    ChartData data;
    std::stringstream ss(osuContent);
    std::string line;
    std::string section = "";

    while (std::getline(ss, line))
    {
        std::string trimmedLine = Utils::trim(line);
        if (trimmedLine.empty() || trimmedLine.rfind("//", 0) == 0)
            continue;

        if (trimmedLine.front() == '[' && trimmedLine.back() == ']')
        {
            section = trimmedLine.substr(1, trimmedLine.size() - 2);
            continue;
        }

        if (section == "Metadata")
        {
            size_t colonPos = trimmedLine.find(':');
            if (colonPos != std::string::npos)
            {
                std::string key = Utils::trim(trimmedLine.substr(0, colonPos));
                std::string val = Utils::trim(trimmedLine.substr(colonPos + 1));
                std::transform(key.begin(), key.end(), key.begin(), ::tolower);

                if (key == "title")
                    data.metadata["title"] = val;
                else if (key == "artist")
                    data.metadata["artist"] = val;
                else if (key == "creator")
                    data.metadata["charter"] = val;
                else if (key == "version")
                    data.metadata["difficulty"] = val;
            }
        }
        else if (section == "General")
        {
            size_t colonPos = trimmedLine.find(':');
            if (colonPos != std::string::npos)
            {
                std::string key = Utils::trim(trimmedLine.substr(0, colonPos));
                std::string val = Utils::trim(trimmedLine.substr(colonPos + 1));
                if (key == "AudioFilename")
                {
                    data.metadata["audio"] = val;
                }
            }
        }
        else if (section == "Events")
        {
            if (trimmedLine.rfind("0,0,", 0) == 0)
            {
                auto parts = Utils::split(trimmedLine, ',');
                if (parts.size() >= 3)
                {
                    std::string bgFile = Utils::trim(parts[2]);
                    if (bgFile.front() == '\"' && bgFile.back() == '\"')
                    {
                        bgFile = bgFile.substr(1, bgFile.size() - 2);
                    }
                    data.metadata["background"] = bgFile;
                }
            }
        }
        else if (section == "Difficulty")
        {
            if (trimmedLine.rfind("CircleSize:", 0) == 0)
            {
                size_t colonPos = trimmedLine.find(':');
                data.keyCount = std::stoi(Utils::trim(trimmedLine.substr(colonPos + 1)));
                data.metadata["keys"] = std::to_string(data.keyCount);
            }
        }
        else if (section == "TimingPoints")
        {
            auto parts = Utils::split(trimmedLine, ',');
            if (parts.size() >= 7 && std::stoi(parts[6]) == 1)
            {
                float time = std::floor(std::stod(parts[0]));
                double msPerBeat = std::stod(parts[1]);
                if (msPerBeat > 0)
                {
                    double bpm = 60000.0 / msPerBeat;
                    data.timingPoints.push_back({time, bpm});
                }
            }
        }
        else if (section == "HitObjects")
        {
            auto parts = Utils::split(trimmedLine, ',');
            if (parts.size() >= 5)
            {
                int x = std::stoi(parts[0]);
                float time = std::stof(parts[2]);
                int type = std::stoi(parts[3]);

                int column = std::min(static_cast<int>(std::floor(x * data.keyCount / 512.0)), data.keyCount - 1);

                bool isHold = (type & 128) > 0;

                if (isHold && parts.size() >= 6)
                {
                    std::string objectParams = parts[5];
                    size_t colonPos = objectParams.find(':');
                    float endTime = (colonPos != std::string::npos)
                                        ? std::stoi(objectParams.substr(0, colonPos))
                                        : std::stoi(objectParams);

                    if (endTime > time)
                    {
                        data.notes.push_back({time, column, HOLD_START});
                        data.notes.push_back({endTime, column, HOLD_END});
                    }
                }
                else
                {
                    data.notes.push_back({time, column, TAP});
                }
            }
        }
    }

    std::sort(data.notes.begin(), data.notes.end());
    return data;
}

ChartData processSmNotesBlock(const std::string& notesBlock, float offset, const std::vector<TimingPoint>& timingPoints, bool isSSC) {
    ChartData data;
    data.timingPoints = timingPoints;
    
    std::stringstream chart_ss(notesBlock);
    
    if (isSSC) {
        std::string line;
        while (std::getline(chart_ss, line)) {
            line = Utils::trim(line);
            if (line.empty()) continue;
            
            if (line.front() == '#') {
                if (line.back() == ';') {
                    line = line.substr(0, line.length() - 1);
                    line = Utils::trim(line);
                }
                
                size_t colonPos = line.find(':');
                if (colonPos != std::string::npos) {
                    std::string tag = line.substr(1, colonPos - 1);
                    std::string value = line.substr(colonPos + 1);
                    
                    tag = Utils::trim(tag);
                    value = Utils::trim(value);
                    
                    if (tag == "STEPSTYPE") {
                        data.metadata["mode"] = value;
                        if (value == "dance-single") data.keyCount = 4;
                        else if (value == "dance-double") data.keyCount = 8;
                        data.metadata["keys"] = std::to_string(data.keyCount);
                    } else if (tag == "DESCRIPTION") {
                        data.metadata["charter"] = value;
                    } else if (tag == "DIFFICULTY") {
                        data.metadata["difficulty"] = value;
                    } else if (tag == "NOTES") {
                        std::string chartNotesData;
                        std::string tempLine;
                        while (std::getline(chart_ss, tempLine)) {
                            chartNotesData += tempLine + "\n";
                        }
                        
                        std::string chartNotesDataCleaned;
                        for (char c : chartNotesData) {
                            if (c == '\r') continue;
                            chartNotesDataCleaned += c;
                        }
                        
                        size_t npos = chartNotesDataCleaned.find("\n\n");
                        while (npos != std::string::npos) {
                            chartNotesDataCleaned.replace(npos, 2, "\n");
                            npos = chartNotesDataCleaned.find("\n\n");
                        }
                        
                        std::stringstream measure_ss(chartNotesDataCleaned);
                        std::string measureStr;
                        int measureIndex = 0;
                        
                        while (std::getline(measure_ss, measureStr, ',')) {
                            measureStr = Utils::trim(measureStr);
                            
                            std::stringstream row_ss(measureStr);
                            std::string rowStr;
                            std::vector<std::string> rows;
                            
                            while (std::getline(row_ss, rowStr, '\n')) {
                                rowStr = Utils::trim(rowStr);
                                
                                bool hasNoteChar = false;
                                for (char c : rowStr) {
                                    if (c == '0' || c == '1' || c == '2' || c == '3' || c == '4' || c == 'M') {
                                        hasNoteChar = true;
                                        break;
                                    }
                                }
                                
                                if (!rowStr.empty() && hasNoteChar) {
                                    rows.push_back(rowStr);
                                }
                            }
                            
                            int rowsPerMeasure = rows.size();
                            
                            if (rowsPerMeasure == 0) {
                                measureIndex++;
                                continue;
                            }

                            float measureLengthBeats = 4.0f; 
                            
                            for (int rowIndex = 0; rowIndex < rowsPerMeasure; ++rowIndex) {
                                const std::string& noteRow = rows[rowIndex];
                                
                                float beat = (float)measureIndex * measureLengthBeats + 
                                             (float)rowIndex * (measureLengthBeats / (float)rowsPerMeasure);
                                
                                float time = calculateBeatsToSeconds(beat, offset, data.timingPoints) * 1000.0f;
                                
                                for (int col = 0; col < std::min((int)noteRow.length(), data.keyCount); ++col) {
                                    char noteTypeChar = noteRow[col];
                                    NoteType noteType = TAP;
                                    bool isNote = true;

                                    switch (noteTypeChar) {
                                        case '1': noteType = TAP; break;
                                        case '2': noteType = HOLD_START; break;
                                        case '3': noteType = HOLD_END; break;
                                        case 'M': noteType = MINE; break;
                                        default: isNote = false; break;
                                    }
                                    
                                    if (isNote) {
                                        data.notes.push_back({time, col, noteType});
                                    }
                                }
                            }

                            measureIndex++;
                        }
                        break;
                    }
                }
            }
        }
    } else {
        std::vector<std::string> properties;
        std::string accumulated;
        
        int colonCount = 0;
        char ch;
        while (chart_ss.get(ch) && colonCount < 5) {
            if (ch == ':') {
                properties.push_back(Utils::trim(accumulated));
                accumulated.clear();
                colonCount++;
            } else {
                accumulated += ch;
            }
        }

        if (properties.size() >= 5) {
            data.metadata["mode"] = properties[0];
            if (properties[0] == "dance-single") data.keyCount = 4;
            else if (properties[0] == "dance-double") data.keyCount = 8;
            data.metadata["keys"] = std::to_string(data.keyCount);
            
            data.metadata["charter"] = properties[1];
            data.metadata["difficulty"] = properties[2];
        }
        
        std::string chartNotesData;
        std::string tempLine;
        while (std::getline(chart_ss, tempLine)) {
            chartNotesData += tempLine + "\n";
        }
        
        std::string chartNotesDataCleaned;
        for (char c : chartNotesData) {
            if (c == '\r') continue;
            chartNotesDataCleaned += c;
        }
        
        size_t npos = chartNotesDataCleaned.find("\n\n");
        while (npos != std::string::npos) {
            chartNotesDataCleaned.replace(npos, 2, "\n");
            npos = chartNotesDataCleaned.find("\n\n");
        }
        
        std::stringstream measure_ss(chartNotesDataCleaned);
        std::string measureStr;
        int measureIndex = 0;
        
        while (std::getline(measure_ss, measureStr, ',')) {
            measureStr = Utils::trim(measureStr);
            
            std::stringstream row_ss(measureStr);
            std::string rowStr;
            std::vector<std::string> rows;
            
            while (std::getline(row_ss, rowStr, '\n')) {
                rowStr = Utils::trim(rowStr);
                
                bool hasNoteChar = false;
                for (char c : rowStr) {
                    if (c == '0' || c == '1' || c == '2' || c == '3' || c == '4' || c == 'M') {
                        hasNoteChar = true;
                        break;
                    }
                }
                
                if (!rowStr.empty() && hasNoteChar) {
                    rows.push_back(rowStr);
                }
            }
            
            int rowsPerMeasure = rows.size();
            
            if (rowsPerMeasure == 0) {
                measureIndex++;
                continue;
            }

            float measureLengthBeats = 4.0f; 
            
            for (int rowIndex = 0; rowIndex < rowsPerMeasure; ++rowIndex) {
                const std::string& noteRow = rows[rowIndex];
                
                float beat = (float)measureIndex * measureLengthBeats + 
                             (float)rowIndex * (measureLengthBeats / (float)rowsPerMeasure);
                
                float time = calculateBeatsToSeconds(beat, offset, data.timingPoints) * 1000.0f;
                
                for (int col = 0; col < std::min((int)noteRow.length(), data.keyCount); ++col) {
                    char noteTypeChar = noteRow[col];
                    NoteType noteType = TAP;
                    bool isNote = true;

                    switch (noteTypeChar) {
                        case '1': noteType = TAP; break;
                        case '2': noteType = HOLD_START; break;
                        case '3': noteType = HOLD_END; break;
                        case 'M': noteType = MINE; break;
                        default: isNote = false; break;
                    }
                    
                    if (isNote) {
                        data.notes.push_back({time, col, noteType});
                    }
                }
            }

            measureIndex++;
        }
    }

    std::sort(data.notes.begin(), data.notes.end());
    return data;
}

std::vector<ChartData> ChartUtils::convertSmToChartDataMultiple(const std::string& smContent) {
    std::vector<ChartData> charts;
    std::stringstream ss(smContent);
    std::string line;
    
    float offset = 0.0f;
    std::vector<TimingPoint> timingPoints;
    std::map<std::string, std::string> commonMetadata;
    
    std::vector<std::string> notesBlocks;
    std::string currentNotesBlock;
    bool inNotesSection = false;
    bool isSSC = smContent.find("#NOTEDATA:") != std::string::npos;

    while (std::getline(ss, line)) {
        size_t commentPos = line.find("//");
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }
        
        line = Utils::trim(line);
        if (line.empty()) continue;

        if (isSSC && line.find("#NOTEDATA:") != std::string::npos) {
            if (inNotesSection && !currentNotesBlock.empty()) {
                notesBlocks.push_back(currentNotesBlock);
            }
            inNotesSection = true;
            currentNotesBlock.clear();
            continue;
        }
        
        if (line.find("#NOTES:") != std::string::npos) {
            if (inNotesSection && !currentNotesBlock.empty()) {
                notesBlocks.push_back(currentNotesBlock);
            }
            inNotesSection = true;
            currentNotesBlock.clear();
            continue;
        }

        if (inNotesSection) {
            currentNotesBlock += line + "\n";
            
            if (line.find(';') != std::string::npos) {
                size_t semiPos = currentNotesBlock.find(';');
                if (semiPos != std::string::npos) {
                    currentNotesBlock = currentNotesBlock.substr(0, semiPos);
                }
                notesBlocks.push_back(currentNotesBlock);
                currentNotesBlock.clear();
                inNotesSection = false;
            }
            continue;
        }

        if (line.front() == '#') {
            if (line.back() == ';') {
                line = line.substr(0, line.length() - 1);
                line = Utils::trim(line);
            }
            
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                std::string tag = line.substr(1, colonPos - 1);
                std::string value = line.substr(colonPos + 1);

                tag = Utils::trim(tag);
                value = Utils::trim(value);
                
                if (tag == "OFFSET") {
                    try {
                        offset = std::stof(value);
                    } catch (...) {}
                } else if (tag == "BPMS") {
                    while (value.back() != ';' && std::getline(ss, line)) {
                        line = Utils::trim(line);
                        if (!line.empty()) {
                            value += line;
                        }
                    }
                    
                    if (value.back() == ';') {
                        value = value.substr(0, value.length() - 1);
                    }
                    
                    std::string cleanValue = value;
                    std::replace(cleanValue.begin(), cleanValue.end(), '\n', ',');
                    std::replace(cleanValue.begin(), cleanValue.end(), '\r', ',');

                    std::stringstream bpmss(cleanValue);
                    std::string entry;
                    
                    while (std::getline(bpmss, entry, ',')) {
                        entry = Utils::trim(entry);
                        if (entry.empty()) continue;
                        
                        size_t eqPos = entry.find('=');
                        if (eqPos != std::string::npos) {
                            try {
                                float beat = std::stof(entry.substr(0, eqPos)); 
                                double val = std::stod(entry.substr(eqPos + 1));
                                timingPoints.push_back({beat, val});
                            } catch (...) {}
                        }
                    }
                    
                    std::sort(timingPoints.begin(), timingPoints.end(), 
                              [](const TimingPoint& a, const TimingPoint& b) {
                        return a.time < b.time;
                    });
                } else {
                    std::string key = Utils::trim(tag);
                    if (key == "TITLE") commonMetadata["title"] = value;
                    else if (key == "ARTIST") commonMetadata["artist"] = value;
                    else if (key == "CREDIT") commonMetadata["charter"] = value;
                    else if (key == "MUSIC") commonMetadata["audio"] = value;
                    else if (key == "BACKGROUND") commonMetadata["background"] = value;
                    else if (key == "SAMPLESTART") commonMetadata["previewTime"] = value;
                    else if (key == "SAMPLELENGTH") commonMetadata["previewLength"] = value;
                    else if (key == "TITLETRANSLIT" && !value.empty()) commonMetadata["title"] = value;
                    else if (key == "ARTISTTRANSLIT" && !value.empty()) commonMetadata["artist"] = value;
                }
            }
        }
    }

    if (inNotesSection && !currentNotesBlock.empty()) {
        notesBlocks.push_back(currentNotesBlock);
    }

    for (const auto& notesBlock : notesBlocks) {
        ChartData chart = processSmNotesBlock(notesBlock, offset, timingPoints, isSSC);
        
        for (const auto& kv : commonMetadata) {
            if (chart.metadata.find(kv.first) == chart.metadata.end()) {
                chart.metadata[kv.first] = kv.second;
            }
        }
        
        charts.push_back(chart);
    }

    return charts;
}

std::map<std::string, ChartData> ChartUtils::parseChartMultiple(const std::string &filePath, const std::string &filename, const std::string &content)
{
    std::map<std::string, ChartData> charts;

    if (Utils::hasEnding(filename, ".osu"))
    {
        ChartData data = convertOsuToChartData(content);
        data.filename = filename;
        
        std::string diffName = "Default";
        auto it = data.metadata.find("difficulty");
        if (it != data.metadata.end() && !it->second.empty()) {
            diffName = it->second;
        }
        
        charts[diffName] = data;
    }
    else if (Utils::hasEnding(filename, ".sm") || Utils::hasEnding(filename, ".ssc"))
    {
        std::vector<ChartData> multipleCharts = convertSmToChartDataMultiple(content);
        
        for (auto& chart : multipleCharts) {
            chart.filename = filename;
            
            std::string diffName = "Default";
            auto it = chart.metadata.find("difficulty");
            if (it != chart.metadata.end() && !it->second.empty()) {
                diffName = it->second;
            }
            
            charts[diffName] = chart;
        }
    }
    else if (Utils::hasEnding(filename, ".vsc"))
    {
        ChartData data = parseVsc(content);
        data.filename = filename;
        
        std::string diffName = "Default";
        auto it = data.metadata.find("difficulty");
        if (it != data.metadata.end() && !it->second.empty()) {
            diffName = it->second;
        }
        
        charts[diffName] = data;
    }
    else
    {
        GAME_LOG_WARN("Unsupported chart format for file: " + filename);
    }

    return charts;
}

ChartData ChartUtils::parseChart(const std::string &filePath, const std::string &filename, const std::string &content)
{
    ChartData data;

    if (Utils::hasEnding(filename, ".osu"))
    {
        data = convertOsuToChartData(content);
    }
    else if (Utils::hasEnding(filename, ".sm") || Utils::hasEnding(filename, ".ssc"))
    {
        std::vector<ChartData> charts = convertSmToChartDataMultiple(content);
        if (!charts.empty()) {
            data = charts[0];
        }
    }
    else if (Utils::hasEnding(filename, ".vsc"))
    {
        data = parseVsc(content);
    }
    else
    {
        GAME_LOG_WARN("Unsupported chart format for file: " + filename);
    }

    data.filename = filename;
    return data;
}