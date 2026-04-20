#ifndef _LOGGER_H
#define _LOGGER_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <ctime>
#include <string>

enum severity_level { trace, debug, info, warning, error, fatal };

using namespace std;
inline const char *to_string(severity_level lvl) {
	switch (lvl) {
	case trace: return "[trace]";
	case debug: return "[debug]";
	case info: return "[info]";
	case warning: return "[warning]";
	case error: return "[error]";
	case fatal: return "[fatal]";
	default: return "[unknown]";
	}
}

inline string get_time() {
	char buf[32] = {0};
	time_t now = time(nullptr);
#if defined(_WIN32)
	tm local_tm;
	localtime_s(&local_tm, &now);
	strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &local_tm);
#else
	tm local_tm;
	localtime_r(&now, &local_tm);
	strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &local_tm);
#endif
	return string(buf);
}
template < typename CharT, typename TraitsT >
inline basic_ostream< CharT, TraitsT > &operator<<(basic_ostream< CharT, TraitsT > &strm, severity_level lvl) {
	strm << get_time() << " " << to_string(lvl);
	return strm;
}

class LogStream {
public:
	LogStream(severity_level lvl, const char* file, int line, const char* func) {
		cout << lvl << " " << file << " " << line << " " << func << " ";
	}
	~LogStream() {
		cout << std::endl;
	}
	template<typename T>
	LogStream& operator<<(const T& value) {
		cout << value;
		return *this;
	}
};

#define BLOG(level) LogStream(level, __FILE__, __LINE__, __func__)

#endif