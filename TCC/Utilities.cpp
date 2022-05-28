#include "Pch.hpp"
#include "Utilities.hpp"

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

bool TryUpdateClosestCircle(const vec2d& coordDouble, double& curMinD2, const vec2d& coord, double r)
{
	auto d2 = square(coord - coordDouble);
	if (d2 <= square(r)) // mouse dentro do círculo
		if (d2 < curMinD2) // círculo de raio mais próximo
		{
			curMinD2 = d2;
			return true;
		}
	return false;
}

void HelpMarker(const char* desc)
{
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}