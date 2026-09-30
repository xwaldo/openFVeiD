#pragma once

#include "imgui.h"

namespace UiTheme {

enum class TextRole {
    Error,
    Warning,
    Success,
    Info
};

ImVec4 textColor(TextRole role);

}
