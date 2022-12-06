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

bool TryUpdateClosestCircle(const vec2d& coordDouble, float& curMinD2, const vec2d& coord, float radius)
{
	auto d2 = square(coord - coordDouble);
	if (d2 <= square(radius)) // mouse dentro do círculo
		if (d2 < curMinD2) // círculo de raio mais próximo
		{
			curMinD2 = (float)d2;
			return true;
		}
	return false;
}

bool TryUpdateClosestCircle(const vec2d& coordDouble, float& curMinD2, ElementInteractable& e)
{
	return TryUpdateClosestCircle(coordDouble, curMinD2, e.coord, e.radius);
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

double AreaTrig(const vec2d& p1, const vec2d& p2, const vec2d& p3)
{
	return .5 * TwiceAreaPolygonSigned(std::array<vec2d, 3>{ p1, p2, p3 });
}

bool ColorPicker3U32(const char* label, sf::Color& c)
{
	float col[]
	{
		c.r / 255.f,
		c.g / 255.f,
		c.b / 255.f
	};
	bool result = ImGui::ColorEdit3(label, col);
	c = ToSFMLColor(col);
	return result;
}

sf::Vector2f ToSFML(const vec2f& v)
{
	return sf::Vector2f(v.x, v.y);
}


void PrepareCircleRadius(sf::CircleShape& circle, float r)
{
	circle.setRadius(r);
	circle.setOrigin(r, r);
}

sf::Vector2f ToSFML(const RVO::Vector2& v)
{
	return { v.x(), v.y() };
}