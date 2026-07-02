#ifndef FUNCTION_H
#define FUNCTION_H

/*
#    FVD++, an advanced coaster design tool for NoLimits
#    Copyright (C) 2012-2015, Stephan "Lenny" Alt <alt.stephan@web.de>
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
#    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "subfunction.h"
#include <vector>
#include <fstream>

enum eFunctype { funcRoll,
                 funcNormal,
                 funcLateral,
                 funcPitch,
                 funcYaw };

class section;

class func {
public:
    func(double min, double max, double start, double end, section* _parent,
         enum eFunctype newtype);
    ~func();
    void appendSubFunction(double length, int i = -1);
    void removeSubFunction(int i = -1);

    double getValue(double x);

    void setMaxArgument(double newMax);
    double getMaxArgument() const {
        return funcList[funcList.size() - 1]->maxArgument;
    }

    double getMaxValue();
    double getMinValue();

    void translateValues(subfunc* caller);

    double changeLength(double newlength, int index);

    int getSubfuncNumber(subfunc* _sub);

    void saveFunction(std::ostream& file);
    void loadFunction(std::istream& file);

    bool unlock(int _id);
    bool lock(int _id);

    int lockedFunc();
    subfunc* getSubfunc(double x);

    std::vector<subfunc*> funcList;

    int activeSubfunc;
    const enum eFunctype type;
    section* const secParent;

private:
    double startValue;
};

#endif // FUNCTION_H
