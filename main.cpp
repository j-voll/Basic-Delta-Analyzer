#include <iostream>
#include <limits>
#include <algorithm>
#include "datapostprocessor.h"

void printParameters(const std::vector<std::string>& params) {
    std::cout << "Available parameters:\n";
    for (const auto& param : params) {
        std::cout << "  " << param << "\n";
    }
}

std::string getValidParameter(const std::vector<std::string>& validParams, const std::string& prompt) {
    std::string param;
    while (true) {
        std::cout << prompt;
        std::cin >> param;
        
        if (std::find(validParams.begin(), validParams.end(), param) != validParams.end()) {
            return param;
        }
        std::cout << "Invalid parameter. Please choose from the list above.\n";
    }
}

int main() {
    std::cout << "Starting program..." << std::endl;
    std::string filename = "data.csv";
    DataPostProcessor processor;

    try {
        std::cout << "Attempting to read CSV file: " << filename << std::endl;
        processor.readCSV(filename);
        std::cout << "CSV file loaded successfully" << std::endl;
        
        auto validParams = processor.getParameterNames();
        printParameters(validParams);

        char compareParams;
        std::cout << "\nWould you like to compare parameters? (y/n): ";
        std::cin >> compareParams;
        
        if (tolower(compareParams) == 'y') {
            double xPoint;
            std::cout << "Enter X-axis point for comparison: ";
            std::cin >> xPoint;

            std::string param1 = getValidParameter(validParams, "Enter first parameter name: ");
            std::string param2 = getValidParameter(validParams, "Enter second parameter name: ");

            try {
                processor.compareParameterDeltas(xPoint, param1, param2);
            } catch (const std::exception& e) {
                std::cerr << "Error comparing parameters: " << e.what() << std::endl;
            }
        }

        try {
            std::cout << "\nGenerating overview graphs..." << std::endl;
            processor.generateGraphs(0, processor.getDataSize() - 1);
            std::cout << "Graphs generated successfully" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Failed to generate graphs: " << e.what() << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "\nProgram completed. Press Enter to exit..." << std::endl;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
    return 0;
}