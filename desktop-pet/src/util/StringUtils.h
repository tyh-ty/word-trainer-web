#pragma once
#include <string>
#include <vector>

namespace StringUtils {

std::wstring utf8ToWide(const std::string& utf8);
std::string wideToUtf8(const std::wstring& wide);

}
