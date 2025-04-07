#include "datapostprocessor.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cstdlib>

void DataPostProcessor::readCSV(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    std::string line;
    std::vector<std::string> parameters;
    std::vector<std::vector<double>> columnData;
    size_t dataRowCount = 0;

    if (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string cell;
        
        while (std::getline(ss, cell, ',')) {
            parameters.push_back(cell);
            columnData.push_back(std::vector<double>());
        }

        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string cell;
            int colIndex = 0;

            while (std::getline(ss, cell, ',') && colIndex < parameters.size()) {
                try {
                    if (!cell.empty()) {
                        columnData[colIndex].push_back(std::stod(cell));
                    }
                } catch (const std::exception& e) {
                    // Skip non-numeric cells
                }
                colIndex++;
            }
            dataRowCount = std::max(dataRowCount, columnData[0].size());
        }
    }

    m_data.clear();
    for (size_t row = 0; row < dataRowCount; row++) {
        DataPoint point;
        point.x = columnData[0][row];  // First column is X value

        for (size_t col = 1; col < parameters.size(); col++) {
            if (row < columnData[col].size()) {
                point.values[parameters[col]] = columnData[col][row];
            }
        }
        m_data.push_back(point);
    }

    if (m_data.empty()) {
        throw std::runtime_error("No data read from file: " + filename);
    }
}

void DataPostProcessor::compareParameterDeltas(double xPoint, const std::string& param1, const std::string& param2) {
    if (m_data.empty()) {
        throw std::runtime_error("No data available for comparison");
    }

    // Create temporary data file with deltas
    std::ofstream dataFile("delta_temp.txt");
    dataFile << "# X " << param1 << " " << param2 << " Delta\n";
    
    std::vector<double> deltas;
    for (const auto& point : m_data) {
        double value1 = point.values.at(param1);
        double value2 = point.values.at(param2);
        double delta = value2 - value1;
        dataFile << point.x << " " << value1 << " " << value2 << " " << delta << "\n";
        deltas.push_back(delta);
    }
    dataFile.close();

    // Find closest point for specified X
    auto closestPoint = std::lower_bound(m_data.begin(), m_data.end(), xPoint,
        [](const DataPoint& point, double value) { return point.x < value; });
    
    if (closestPoint == m_data.end()) {
        throw std::runtime_error("X point beyond data range");
    }

    double value1 = closestPoint->values.at(param1);
    double value2 = closestPoint->values.at(param2);
    double delta = value2 - value1;

    // Output numeric values
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "At X = " << xPoint << ":\n"
              << param1 << ": " << value1 << "\n"
              << param2 << ": " << value2 << "\n"
              << "Delta: " << delta << "\n";

    // Find y-axis range for delta plot
    auto [minDelta, maxDelta] = std::minmax_element(deltas.begin(), deltas.end());
    double deltaRange = *maxDelta - *minDelta;
    double ymin = *minDelta - deltaRange * 0.1;
    double ymax = *maxDelta + deltaRange * 0.1;

    // Create gnuplot script
    std::string filename = "delta_comparison_" + std::to_string(xPoint);
    std::ofstream script(filename + ".gp");
    script << "set terminal png size 1200,800\n"
           << "set output '" << filename << ".png'\n"
           << "set multiplot layout 2,1\n"
           << "set grid\n"
           << "set key right\n\n"
           
           // Top plot - original curves
           << "set title 'Parameter Values'\n"
           << "set xlabel 'X'\n"
           << "set ylabel 'Value'\n"
           << "plot 'delta_temp.txt' using 1:2 title '" << param1 << "' with lines,\\\n"
           << "     'delta_temp.txt' using 1:3 title '" << param2 << "' with lines,\\\n"
           << "     " << xPoint << "," << value1 << " title 'Selected Points' with points pt 7 ps 2,\\\n"
           << "     " << xPoint << "," << value2 << " notitle with points pt 7 ps 2\n\n"
           
           // Bottom plot - delta
           << "set title 'Delta (" << param2 << " - " << param1 << ")'\n"
           << "set xlabel 'X'\n"
           << "set ylabel 'Delta'\n"
           << "set yrange [" << ymin << ":" << ymax << "]\n"
           << "plot 'delta_temp.txt' using 1:4 title 'Delta' with lines,\\\n"
           << "     " << xPoint << "," << delta << " title sprintf('Delta at X=" 
           << xPoint << ": %.3f', " << delta << ") with points pt 7 ps 2\n";
    
    script.close();

    std::system(("gnuplot " + filename + ".gp").c_str());
    std::remove("delta_temp.txt");
}

void DataPostProcessor::generateGraphs(int startIndex, int endIndex) {
    if (m_data.empty()) {
        throw std::runtime_error("No data available for plotting");
    }

    std::cout << "Starting graph generation..." << std::endl;
    
    std::system("mkdir plots >nul 2>&1");

    std::vector<std::string> validParams;
    for (const auto& [param, _] : m_data[0].values) {
        validParams.push_back(param);
    }

    const size_t BATCH_SIZE = 40;
    size_t totalBatches = (validParams.size() + BATCH_SIZE - 1) / BATCH_SIZE;
    
    for (size_t batch = 0; batch < totalBatches; ++batch) {
        size_t batchStart = batch * BATCH_SIZE;
        size_t batchEnd = std::min(batchStart + BATCH_SIZE, validParams.size());
        
        std::string filename = "plots/batch_" + std::to_string(batch);
        std::ofstream dataFile(filename + ".txt");
        for (const auto& point : m_data) {
            dataFile << point.x;
            for (size_t i = batchStart; i < batchEnd; ++i) {
                dataFile << " " << point.values.at(validParams[i]);
            }
            dataFile << "\n";
        }
        dataFile.close();

        std::ofstream script(filename + ".gp");
        script << "set terminal png size 1600,1000\n";
        script << "set output '" << filename << ".png'\n";
        script << "set title 'Parameters Batch " << (batch + 1) << " of " << totalBatches << "'\n";
        script << "set xlabel 'X'\n";
        script << "set ylabel 'Value'\n";
        script << "set grid\n";
        script << "set key outside right\n";

        script << "plot ";
        for (size_t i = batchStart; i < batchEnd; ++i) {
            if (i > batchStart) script << ", ";
            script << "'" << filename << ".txt' using 1:" << (i - batchStart + 2) 
                   << " with lines title '" << validParams[i] << "'";
        }
        script << "\n";
        script.close();

        std::system(("gnuplot " + filename + ".gp").c_str());
    }
}

std::vector<std::string> DataPostProcessor::getParameterNames() const {
    std::vector<std::string> names;
    if (!m_data.empty()) {
        for (const auto& [param, _] : m_data[0].values) {
            names.push_back(param);
        }
    }
    return names;
}