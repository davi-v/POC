#pragma once

time_t GetCurTime();

bool TimeToFormat(time_t t, const char* format, std::string& out);
std::string TimeToDay(time_t t);

std::string TimeToHour(time_t t);

std::string GetUniqueNameWithCurrentTime(const std::string& prepend, const std::string& ext);