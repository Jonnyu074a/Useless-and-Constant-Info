#pragma once
#include <string>
#include <unordered_map>
#include <ctime>
#include "../../include/UACI.hpp"

namespace uaci {

using TagMap = std::unordered_map<std::string, std::string>;

std::string formatTime(bool is24h, bool showSeconds);
std::string formatDate(int style);
std::string formatSession(std::time_t start, bool showSeconds);
std::string formatTotalSession();

// Single-pass renderer:
//  - splits `fmt` on `sep` into segments
//  - replaces {tags} using `vals` (unknown tags are dropped; values are NEVER re-scanned,
//    so a level named "{cps}" or "a | b" can't break the layout)
//  - drops segments that render empty
//  - joins with `sep`, or with '\n' when `vertical`
std::string renderFormat(std::string const& fmt, std::string const& sep,
                         bool vertical, TagMap const& vals);

// Sample values used by the Builder preview.
TagMap sampleTags();

// Name/version of a locally installed mod menu ("" if none).
std::string getModMenuDisplay();

int getTargetFPS();

} // namespace uaci
