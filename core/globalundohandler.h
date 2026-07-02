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

#ifndef GLOBALUNDOHANDLER_H
#define GLOBALUNDOHANDLER_H

#include <vector>
#include <string>

class Application;

class GlobalUndoHandler {
public:
    GlobalUndoHandler(Application* app, int stackSize = 64);
    ~GlobalUndoHandler();

    void pushSnapshot();
    void doUndo();
    void doRedo();
    void clearActions();
    void setMaxStackSize(int size) {
        maxStackSize = size;
    }

    bool canUndo() const {
        return stackIndex > 0;
    }
    bool canRedo() const {
        return stackIndex < (int)stack.size() - 1;
    }

    bool busy;

private:
    Application* mApp;
    std::vector<std::string> stack;
    int stackIndex;
    int maxStackSize;
};

#endif // GLOBALUNDOHANDLER_H
