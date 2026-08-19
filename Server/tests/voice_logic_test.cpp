#include <cassert>
#include <cctype>
#include <iostream>
#include <regex>
#include <string>
#include <algorithm>

static bool oldDetectStop(const std::string& text) {
    std::string normalized = text;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return normalized.find("stop") != std::string::npos;
}

static bool newDetectStop(const std::string& text) {
    std::string normalized = text;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    std::regex stop_regex(R"(\bstop\b|\bstop talking\b|\bstop please\b|\bbe quiet\b|\bcancel\b|\bsilence\b)", std::regex_constants::icase);
    return std::regex_search(normalized, stop_regex);
}

static bool isMeaningfulTranscript(const std::string& text) {
    std::string cleaned = text;
    cleaned.erase(std::remove_if(cleaned.begin(), cleaned.end(), [](unsigned char c){ return std::isspace(c) || c == '-' || c == '.'; }), cleaned.end());
    return !cleaned.empty();
}

static bool containsKeywordMatch(const std::string& text, const std::string& keyword) {
    std::string normalized_text = text;
    std::transform(normalized_text.begin(), normalized_text.end(), normalized_text.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    std::string normalized_keyword = keyword;
    std::transform(normalized_keyword.begin(), normalized_keyword.end(), normalized_keyword.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return normalized_text.find(normalized_keyword) != std::string::npos;
}

int main() {
    assert(oldDetectStop("stop") == true);
    assert(oldDetectStop("stopped") == true);
    assert(oldDetectStop("stoplight") == true);
    assert(newDetectStop("stopped") == false);
    assert(newDetectStop("stoplight") == false);
    assert(newDetectStop("stop") == true);
    assert(newDetectStop("be quiet") == true);
    assert(!isMeaningfulTranscript("-"));
    assert(!isMeaningfulTranscript("   "));
    assert(isMeaningfulTranscript("introduce yourself"));
    assert(containsKeywordMatch("- please see a start of the journal education.", "start"));
    assert(containsKeywordMatch("let's start to begin health education.", "start"));
    assert(containsKeywordMatch("introduce yourself", "introduce"));
    std::cout << "voice logic regression checks passed" << std::endl;
    return 0;
}
