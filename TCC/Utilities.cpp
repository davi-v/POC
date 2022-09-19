#include "Pch.hpp"
#include "Utilities.hpp"

sf::Clock globalClock;

time_t GetCurTime()
{
	auto p = std::chrono::utc_clock::now();
	using seconds = std::chrono::seconds;
	return duration_cast<seconds>(p.time_since_epoch()).count();
}

sf::Vector2f ToFloat(const sf::Vector2u& p)
{
	return { (float)p.x, (float)p.y };
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

bool NotHoveringIMGui()
{
	return !ImGui::IsWindowHovered(
		ImGuiHoveredFlags_AnyWindow |
		//ImGuiHoveredFlags_DockHierarchy |
		ImGuiHoveredFlags_AllowWhenBlockedByPopup |
		ImGuiHoveredFlags_AllowWhenBlockedByActiveItem
	);
}

sf::Color ToSFMLColor(float backgroundColor[3])
{
	return {
		static_cast<sf::Uint8>(backgroundColor[0] * 255),
		static_cast<sf::Uint8>(backgroundColor[1] * 255),
		static_cast<sf::Uint8>(backgroundColor[2] * 255),
	};
}

bool ShowRadiusOption(float& r, float maxR)
{
	return ImGui::SliderFloat("Radius", &r, .5f, maxR);
}

double cross(const vec2d& a, const vec2d& b)
{
	return a.x * b.y - a.y * b.x;
}

bool IsInsideConvexShape(const vec2d& p, const std::vector<vec2d>& v)
{
	const auto nEdges = v.size();
	for (size_t i = 0; i != nEdges; i++)
	{
		const auto& prev = v[(i - 1 + nEdges) % nEdges];
		const auto& cur = v[i];
		if (cross(cur - prev, p - prev) < 0)
			return false;
	}
	return true;
}

std::vector<vec2d> CalculateCentroids(const std::vector<Cell>& cells)
{
	const auto n = cells.size();
	std::vector<vec2d> ret(n);
	for (size_t i = 0; i != n; i++)
		ret[i] = CalculateCentroid(cells[i]);
	return ret;
}

// shoelace formula
double TwiceAreaPolygonSigned(const Cell& cell)
{
	double ret = 0;
	const auto n = cell.size();
	for (size_t i = 0; i != n; i++)
	{
		const auto& cur = cell[i];
		const auto& nxt = cell[(i + 1) % n];
		ret += cross(cur, nxt);
	}
	return ret;
}

double AreaTrig(const vec2d& p1, const vec2d& p2, const vec2d& p3)
{
	return .5 * TwiceAreaPolygonSigned({ p1, p2, p3 });
}

vec2d CalculateCentroid(const Cell& cell)
{
	double sx = 0, sy = 0;
	const auto n = cell.size();
	for (size_t i = 0; i != n; i++)
	{
		const auto& cur = cell[i];
		const auto& nxt = cell[(i + 1) % n];
		const auto c = cross(cur, nxt);
		sx += (cur.x + nxt.x) * c;
		sy += (cur.y + nxt.y) * c;
	}
	auto invM = 1. / (3 * TwiceAreaPolygonSigned(cell));
	return { sx * invM, sy * invM };
}
bool ColorPicker3U32(const char* label, sf::Color& c, ImGuiColorEditFlags flags)
{
	float col[3];
	col[0] = c.r / 255.f;
	col[1] = c.g / 255.f;
	col[2] = c.b / 255.f;

	bool result = ImGui::ColorPicker3(label, col, flags);

	c.r = (sf::Uint8)(col[0] * 255.f);
	c.g = (sf::Uint8)(col[1] * 255.f);
	c.b = (sf::Uint8)(col[2] * 255.f);

	return result;
}