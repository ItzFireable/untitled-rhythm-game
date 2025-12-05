#define _USE_MATH_DEFINES

#include "utils/Utils.h"
#include "utils/Logger.h"
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <fstream>
#include <cmath>
#include <sys/stat.h>

static const std::vector<std::string> supportedChartExtensions = {".vsc", ".osu"};

namespace Utils
{
    void getWindowSize(SDL_Window *window, int &width, int &height)
    {
        SDL_GetWindowSize(window, &width, &height);
    }

    void drawCircle(SDL_Renderer *renderer, float centerX, float centerY, float radius, SDL_Color color)
    {
        const int segments = 100;

        Uint8 prevR, prevG, prevB, prevA;
        SDL_GetRenderDrawColor(renderer, &prevR, &prevG, &prevB, &prevA);
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

        for (int i = 0; i < segments; ++i)
        {
            float theta1 = (2.0f * M_PI * i) / segments;
            float theta2 = (2.0f * M_PI * (i + 1)) / segments;

            float x1 = centerX + radius * cosf(theta1);
            float y1 = centerY + radius * sinf(theta1);
            float x2 = centerX + radius * cosf(theta2);
            float y2 = centerY + radius * sinf(theta2);

            SDL_RenderLine(renderer, static_cast<int>(x1), static_cast<int>(y1), static_cast<int>(x2), static_cast<int>(y2));
        }

        SDL_SetRenderDrawColor(renderer, prevR, prevG, prevB, prevA);
    }

    std::string trim(const std::string &str)
    {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (std::string::npos == first)
            return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

    std::vector<std::string> split(const std::string &s, char delimiter)
    {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(s);
        while (std::getline(tokenStream, token, delimiter))
        {
            tokens.push_back(token);
        }
        return tokens;
    }

    std::string formatMemorySize(size_t bytes)
    {
        const char *sizes[] = {"B", "KB", "MB", "GB", "TB"};
        int order = 0;
        double size = static_cast<double>(bytes);
        while (size >= 1024 && order < 4)
        {
            order++;
            size = size / 1024;
        }
        std::ostringstream oss;
        oss.precision(2);
        oss << std::fixed << size << " " << sizes[order];
        return oss.str();
    }

    float getAudioStartPos(const ChartData &chartData)
    {
        auto it = chartData.metadata.find("previewTime");
        if (it != chartData.metadata.end() && !it->second.empty())
        {
            std::string startPosStr = it->second;
            try
            {
                float startPos = std::stof(startPosStr);
                if (startPos >= 0.0f)
                {
                    return startPos;
                }
                else
                {
                    GAME_LOG_ERROR("Invalid start position in metadata: " + startPosStr);
                }
            }
            catch (const std::invalid_argument &e)
            {
                GAME_LOG_ERROR("Invalid start position format: " + std::string(e.what()));
            }
            catch (const std::out_of_range &e)
            {
                GAME_LOG_ERROR("Start position out of range: " + std::string(e.what()));
            }
        }

        return 0.0f;
    }

    std::string getAudioPath(const ChartData &chartData)
    {
        auto it = chartData.metadata.find("audio");
        if (it != chartData.metadata.end() && !it->second.empty())
        {
            std::string audioPath = it->second;

            if (audioPath[0] != '/' && audioPath.find("assets/") != 0)
            {
                std::string chartDir = chartData.filename;
                size_t lastSlash = chartDir.find_last_of("/\\");
                if (lastSlash != std::string::npos)
                {
                    chartDir = chartDir.substr(0, lastSlash + 1);
                    audioPath = chartDir + audioPath;
                }
            }

            return audioPath;
        }

        std::string chartFile = "audio.mp3";
        std::string filePath = chartData.filePath;
        size_t lastSlash = filePath.find_last_of("/\\");

        if (lastSlash != std::string::npos)
        {
            std::string mp3Path = filePath + "/" + chartFile;
            return mp3Path;
        }

        return "";
    }

    std::string getChartAssetPath(const ChartData &chartData, const std::string &assetName)
    {
        std::string chartDir = chartData.filePath;
        size_t lastSlash = chartDir.find_last_of("/\\");
        if (lastSlash != std::string::npos)
        {
            chartDir = chartDir.substr(0, lastSlash + 1);
            return chartDir + assetName;
        }
        return assetName;
    }

    std::vector<std::string> getChartList()
    {
        const std::string &directoryPath = "assets/songs/";
        std::vector<std::string> chartFiles;

        if (!std::filesystem::exists(directoryPath) || !std::filesystem::is_directory(directoryPath))
        {
            return chartFiles;
        }

        for (const auto &entry : std::filesystem::directory_iterator(directoryPath))
        {
            if (entry.is_directory())
            {
                chartFiles.push_back(entry.path().string());
            }
        }
        return chartFiles;
    }

    std::string getChartFile(const std::string &chartDirectory)
    {
        for (const auto &entry : std::filesystem::directory_iterator(chartDirectory))
        {
            if (entry.is_regular_file())
            {
                std::string filePath = entry.path().string();
                for (const auto &ext : supportedChartExtensions)
                {
                    if (hasEnding(filePath, ext))
                    {
                        return filePath;
                    }
                }
            }
        }

        return "";
    }

    bool fileExists(const std::string &path)
    {
#ifdef _WIN32
        struct _stat buffer;
        return (_stat(path.c_str(), &buffer) == 0);
#else
        struct stat buffer;
        return (stat(path.c_str(), &buffer) == 0);
#endif
    }

    bool hasEnding(std::string const &fullString, std::string const &ending)
    {
        if (fullString.length() >= ending.length())
        {
            return std::equal(ending.rbegin(), ending.rend(), fullString.rbegin(), [](char a, char b)
                              { return std::tolower(a) == std::tolower(b); });
        }
        return false;
    }

    std::string readFile(const std::string &fullPath)
    {
        std::ifstream t(fullPath);
        if (!t.is_open())
            return "";

        std::stringstream buffer;
        buffer << t.rdbuf();
        return buffer.str();
    }

    double pNorm(const std::vector<double> &values, const std::vector<double> &weights, double P)
    {
        if (values.empty())
            return 0.0;
        double sum = 0.0;
        for (size_t i = 0; i < values.size(); ++i)
        {
            sum += std::pow(values[i] * weights[i], P);
        }
        return std::pow(sum, 1.0 / P);
    }

    double calculateStandardDeviation(const std::vector<double> &array)
    {
        if (array.size() <= 1)
            return 0.0;
        double sum = std::accumulate(array.begin(), array.end(), 0.0);
        double mean = sum / array.size();
        double sq_sum = 0.0;
        for (double val : array)
            sq_sum += std::pow(val - mean, 2);
        return std::sqrt(sq_sum / array.size());
    }
}