/*
 * xPilot: X-Plane pilot client for VATSIM
 * Copyright (C) 2019-2022 Justin Shannon
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 */

#ifndef Utilities_h
#define Utilities_h

#include "Constants.h"
#include "Config.h"
#include "XPLMPlugin.h"
#include "XPLMUtilities.h"
#include "XPLMDataAccess.h"

#include <ctime>
#include <string>
#include <memory>
#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cstdarg>
#include <cmath>

template <typename... Args>
inline std::string string_format(const std::string& format, Args... args) {
	size_t size = snprintf(nullptr, 0, format.c_str(), args...) + 1;
	std::unique_ptr<char[]> buf(new char[size]);
	snprintf(buf.get(), size, format.c_str(), args...);
	return std::string(buf.get(), buf.get() + size - 1);
}

template <class TContainer>
inline bool begins_with(const TContainer& input, const TContainer& match) {
	return input.size() >= match.size() && std::equal(match.cbegin(), match.cend(), input.cbegin());
}

template <class TContainer>
inline bool ends_with(const TContainer& input, const TContainer& match) {
	return input.size() >= match.size() && input.compare(input.size() - match.size(), match.size(), match) == 0;
}

inline char* strScpy(char* dest, const char* src, size_t size) {
	strncpy(dest, src, size);
	dest[size - 1] = 0;
	return dest;
}

inline std::string strAtMost(const std::string s, size_t m) {
	return s.length() <= m ? s : s.substr(0, m - 3) + "...";
}

#if APL == 1 || LIN == 1
inline void strncpy_s(char* dest, size_t destsz, const char* src, size_t count) {
	strncpy(dest, src, std::min(destsz, count));
	dest[destsz - 1] = 0;
}
#endif

#define STRCPY_ATMOST(dest, src) strncpy_s(dest, sizeof(dest), strAtMost(src, sizeof(dest) - 1).c_str(), sizeof(dest) - 1)

inline const auto str_tolower = [](std::string s) {
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
	return s;
};
inline const auto str_toupper = [](std::string s) {
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::toupper(c); });
	return s;
};

template <class ContainerT>
inline void tokenize(const std::string& str, ContainerT& tokens, const std::string& delimiters = " ", bool trimEmpty = false) {
	std::string::size_type pos, lastPos = 0, length = str.length();

	using value_type = typename ContainerT::value_type;
	using size_type = typename ContainerT::size_type;

	while (lastPos < length + 1) {
		pos = str.find_first_of(delimiters, lastPos);
		if (pos == std::string::npos) {
			pos = length;
		}

		if (pos != lastPos || !trimEmpty)
			tokens.push_back(value_type(str.data() + lastPos,
				(size_type)pos - lastPos));

		lastPos = pos + 1;
	}
}

inline void join(const std::vector<std::string>& v, char c, std::string& s) {
	s.clear();
	auto it = v.begin();
	++it;
	++it;
	for (auto end = v.end(); it != end; ++it) {
		s += *it;
		if (it != v.end() - 1) {
			s += c;
		}
	}
}

inline std::string joinSkipFirst(const std::vector<std::string>& v, const std::string& delimiter = " ") {
	std::string out;
	if (auto i = std::next(v.begin()), e = v.end(); i != e) {
		out += *i++;
		for (; i != e; ++i)
			out.append(delimiter).append(*i);
	}
	return out;
}

inline double Round(double value, int to) {
	double places = pow(10.0, to);
	return round(value * places) / places;
}

inline std::string GetXPlanePath() {
	char buffer[2048];
	XPLMGetSystemPath(buffer);
	return buffer;
}

inline std::string GetPluginPath() {
	XPLMPluginID myId = XPLMGetMyID();
	char buffer[2048];
	XPLMGetPluginInfo(myId, nullptr, buffer, nullptr, nullptr);
	char* path = XPLMExtractFileAndPath(buffer);
	return std::string(buffer, 0, path - buffer) + "/../";
}

inline std::string RemoveSystemPath(std::string path) {
	if (begins_with<std::string>(path, GetXPlanePath())) {
		path.erase(0, GetXPlanePath().length());
	}
	return path;
}

inline int CountFilesInPath(const std::string& path) {
	char buffer[2048];
	int fileCount = 0;
	XPLMGetDirectoryContents(path.c_str(), 0, buffer, sizeof(buffer), nullptr, 0, &fileCount, nullptr);
	return fileCount;
}

inline std::string UtcTimestamp() {
	auto tp = std::chrono::system_clock::now();
	time_t current_time = std::chrono::system_clock::to_time_t(tp);
	tm* timeInfo = gmtime(&current_time);
	char buffer[128];
	return std::string(buffer, buffer + strftime(buffer, sizeof(buffer), "%H:%M:%S", timeInfo));
}

inline float GetNetworkTime() {
	static XPLMDataRef drNetworkTime = nullptr;
	if (!drNetworkTime) {
		drNetworkTime = XPLMFindDataRef("sim/network/misc/network_time_sec");
	}
	return XPLMGetDataf(drNetworkTime);
}

inline int64_t PrecisionTimestamp() {
	using namespace std::chrono;
	steady_clock::duration dur{ steady_clock::now().time_since_epoch() };
	return duration_cast<milliseconds>(dur).count();
}

struct rgb
{
	float r;
	float g;
	float b;
};

inline rgb IntToRgb(int v) {
	rgb result{};
	result.r = (v >> 0) & 255;
	result.g = (v >> 8) & 255;
	result.b = (v >> 16) & 255;
	return result;
}

inline void HexToRgb(int inCol, float outColor[4]) {
	outColor[0] = float((inCol & 0xFF0000) >> 16) / 255.0f;
	outColor[1] = float((inCol & 0x00FF00) >> 8) / 255.0f;
	outColor[2] = float((inCol & 0x0000FF)) / 255.0f;
	outColor[3] = 1.0f;
}

enum logLevel
{
	logDEBUG, // Debug, highest level of detail
	logINFO,	// Regular info messages
	logWARN,	// Warnings, i.e. unexpected, but critical events
	logERROR, // Errors
	logFATAL, // Fatal errors, often results in a crash
	logMSG		// Message will always output, regardless of minimum log level
};

inline const char* LOG_LEVEL[] = { " DEBUG ", " INFO ", " WARN ", " ERROR ", " FATAL ", "" };

inline const char* Logger(logLevel level, const char* msg, va_list args) {
	static char buf[2048];

	float secs = GetNetworkTime();
	const unsigned hours = unsigned(secs / 3600.0f);
	secs -= hours * 3600.0f;
	const unsigned mins = unsigned(secs / 60.0f);
	secs -= mins * 60.0f;

	snprintf(buf, sizeof(buf), "%u:%02u:%06.3f %s: %s", hours, mins, secs, PLUGIN_NAME, LOG_LEVEL[level]);
	if (args) {
		vsnprintf(&buf[strlen(buf)], sizeof(buf) - strlen(buf) - 1, msg, args);
	}
	size_t length = strlen(buf);
	if (buf[length - 1] != '\n') {
		buf[length] = '\n';
		buf[length + 1] = 0;
	}
	return buf;
}

inline void Log(logLevel level, const char* msg, ...) {
	va_list args;
	va_start(args, msg);
	XPLMDebugString(Logger(level, msg, args));
	va_end(args);
}

#define LOG_MSG(lvl, ...)                                \
	{                                                      \
		if (lvl >= xpilot::Config::GetInstance().GetLogLevel()) \
		{                                                    \
			Log(lvl, __VA_ARGS__);                             \
		}                                                    \
	}

#endif // !Utilities_h