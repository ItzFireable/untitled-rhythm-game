#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>
#include <cmath>

#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include "rhythm/ChartManager.h"

namespace Utils {
    void getWindowSize(SDL_Window *window, int &width, int &height);

    std::string trim(const std::string& str);
    std::vector<std::string> split(const std::string& s, char delimiter);
    bool hasEnding(std::string const &fullString, std::string const &ending);

    std::string readFile(const std::string& path);
    std::string formatMemorySize(size_t bytes);

    std::vector<std::string> getChartList();
    std::string getChartFile(const std::string& chartDirectory);
    static bool fileExists(const std::string& path);
    std::string getChartAssetPath(const ChartData &chartData, const std::string &assetName);

    float getAudioStartPos(const ChartData &chartData);
    std::string getAudioPath(const ChartData &chartData);

    double pNorm(const std::vector<double>& values, const std::vector<double>& weights, double P);
    double calculateStandardDeviation(const std::vector<double>& array);

    void drawCircle(SDL_Renderer* renderer, float centerX, float centerY, float radius, SDL_Color color = {255, 255, 255, 255});
}

#endif