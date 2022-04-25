#include "Pch.hpp"

time_t GetCurTime()
{
	auto p = std::chrono::utc_clock::now();
	using seconds = std::chrono::seconds;
	return duration_cast<seconds>(p.time_since_epoch()).count();
}

bool TimeToFormat(time_t t, const char* format, std::string& out)
{
	tm buf;
	if (gmtime_s(&buf, &t)) // Invalid time_t, don't pass to std::put_time
		return false;

	std::ostringstream ostream;
	ostream << std::put_time(&buf, format);
	out = ostream.str();
	return true;
}

std::string TimeToDay(time_t t)
{
	std::string buf;
	TimeToFormat(t, "%d-%m-%y", buf);
	return buf;
}

std::string TimeToHour(time_t t)
{
	std::string buf;
	TimeToFormat(t, "%H-%M-%S", buf);
	return buf;
}

std::string GetUniqueNameWithCurrentTime(const std::string& prepend, const std::string& ext)
{
	auto t = GetCurTime();
	auto day = TimeToDay(t);
	auto hour = TimeToHour(t);

	auto beg = prepend + day + '_' + hour;

	std::string extra;

	size_t cnt = 1;
	while (std::filesystem::exists(beg + extra + ext))
		extra = " (" + std::to_string(++cnt) + ')';

	return beg + extra + ext;
}