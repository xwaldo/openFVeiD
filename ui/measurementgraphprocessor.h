/*
#    FVD++, an advanced coaster design tool
#    Copyright (C) 2026 Veia <h27ck@proton.me>
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef MEASUREMENTGRAPHPROCESSOR_H
#define MEASUREMENTGRAPHPROCESSOR_H

#include <vector>

class trackHandler;
class ResultingGraphProcessor;

struct TrainOffsetGraphData {
    std::vector<double> x;
    std::vector<double> yNormal;
    std::vector<double> yLateral;
};

class MeasurementGraphProcessor {
public:
    MeasurementGraphProcessor() = default;
    ~MeasurementGraphProcessor() = default;

    void update(trackHandler* track, const ResultingGraphProcessor& resultingProcessor);

    const std::vector<TrainOffsetGraphData>& getData() const {
        return trainOffsetGraphs;
    }

private:
    std::vector<TrainOffsetGraphData> trainOffsetGraphs;
};

#endif // MEASUREMENTGRAPHPROCESSOR_H
