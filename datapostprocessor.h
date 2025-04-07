#ifndef DATAPOSTPROCESSOR_H
#define DATAPOSTPROCESSOR_H

#include <string>
#include <vector>
#include <map>
#include <memory>

struct DataPoint {
    double x;
    std::map<std::string, double> values;
};

class DataPostProcessor {
public:
    DataPostProcessor() = default;
    ~DataPostProcessor() = default;

    void compareParameterDeltas(double xPoint, const std::string& param1, const std::string& param2);
    void readCSV(const std::string& filename);
    void generateGraphs(int startIndex, int endIndex);
    const std::vector<DataPoint>& getData() const { return m_data; }
    size_t getDataSize() const { return m_data.size(); }
    std::vector<std::string> getParameterNames() const;

private:
    std::vector<DataPoint> m_data;
    void saveGraph(const std::string& filename, 
                  const std::vector<double>& x, 
                  const std::vector<double>& y,
                  const std::string& title,
                  const std::string& xlabel,
                  const std::string& ylabel);
};

#endif // DATAPOSTPROCESSOR_H