#include "Pch.hpp"
#include "PreviewVoronoi.hpp"

#include "Application.hpp"
#include "Utilities.hpp"

const char* PreviewVoronoi::getTitle()
{
	return "Voronoi Coverage";
}

void PreviewVoronoi::onImgChangeImpl()
{
	MakeUniquePtr(voronoiInfo, viewerBase);
	recalculateVoronoi();
}

void PreviewVoronoi::drawExtraCommonEditor()
{
	auto& w = viewerBase.app->window;
	if (drawRobots)
	{
		if (viewerBase.renderType == ViewerBase::RenderType::RightColor)
		{
			sf::CircleShape circle;
			const auto n = robotCoords.size();
			for (size_t i = 0; i != n; i++)
			{
				const auto& coord = robotCoords[i];
				const auto& r = robotRadius[i];
				PrepareCircleRadius(circle, r);
				auto cf = sf::Vector2f(coord);
				circle.setPosition(cf);
				const auto color = viewerBase.getRightColor(cf);
				circle.setFillColor(color);
				w.draw(circle);
			}
		}
		else
			viewerBase.app->drawCircles(robotCoords, robotRadius, robotColor);
	}
	auto& circle = viewerBase.circle;
	auto tryDrawCircles = [&](bool draw, const std::vector<vec2d>& coords, const sf::Color& color)
	{
		if (draw)
			viewerBase.app->drawCircles(coords, robotRadius, color);
	};
	if (drawVoronoi)
		DrawVoronoiEdges(voronoiInfo->voronoi, lineColor, w);
	tryDrawCircles(drawCenters, voronoiInfo->centroids, sf::Color::Blue);
	tryDrawCircles(drawTargets, voronoiInfo->targets, sf::Color::Cyan);

	if (highlightHoveredRobot)
	{
		if (circleHovered != -1)
		{
			circle.setFillColor(sf::Color::Yellow);
			PrepareCircleRadius(circle, robotRadius[circleHovered]);
			circle.setOutlineThickness(-.1f * robotRadius[circleHovered]);
			circle.setPosition(robotCoords[circleHovered]);
			w.draw(circle);
			circle.setOutlineThickness(0);
		}
	}
	if (highlightHoveredCell)
	{
		const auto point = sf::Mouse::getPosition(w);
		const auto coord = w.mapPixelToCoords(point);
		const auto index = voronoiInfo->findCell(coord.x, coord.y);
		if (index != -1)
		{
			const auto& c = voronoiInfo->voronoiCells[index];
			const auto n = c.size();
			for (size_t i = 0; i != n; i++)
			{
				const auto& p0 = c[i];
				const auto& p1 = c[(i + 1) % c.size()];
				sf::Vertex v[2]
				{
					{ {(float)p0.x, (float)p0.y}, sf::Color::Yellow },
					{ {(float)p1.x, (float)p1.y}, sf::Color::Yellow }
				};
				w.draw(v, 2, sf::Lines);
			}
		}
	}
}

void PreviewVoronoi::clear()
{
	Clear(robotCoords, robotRadiusOffs, robotRadius);
	clearPointersToRobots();
	recalculateVoronoi();
}

void PreviewVoronoi::recalculateVoronoi()
{
	voronoiInfo->update(robotCoords);
}

void PreviewVoronoi::drawUIImpl()
{
	if (ImGui::CollapsingHeader("Rendering"))
	{
		ImGui::Checkbox("Voronoi", &drawVoronoi);
		ImGui::Checkbox("Robots##1", &drawRobots);
		ImGui::Checkbox("Centers", &drawCenters);
		ImGui::Checkbox("Targets", &drawTargets);

		ColorPicker3U32("Line", lineColor);
		ColorPicker3U32("Robots", robotColor);

		if (ImGui::CollapsingHeader("Highlight Hovered"))
		{
			ImGui::Checkbox("Robot", &highlightHoveredRobot);
			ImGui::Checkbox("Cell", &highlightHoveredCell);
		}
	}
	if (ImGui::CollapsingHeader("Robots"))
	{
		static constexpr float
			MIN_R = .5,
			MAX_R = 32;
		if (ImGui::SliderFloat("Radius Multiplier", &viewerBase.radius, MIN_R, MAX_R))
		{
			for (size_t i = 0, lim = robotCoords.size(); i != lim; i++)
				robotRadius[i] = robotRadiusOffs[i] * viewerBase.radius;
		}
		if (!robotCoords.empty())
			if (ImGui::Button("Clear"))
				clear();
	}
	if (ImGui::CollapsingHeader("Navigator"))
	{
		if (ImGui::SliderFloat("Speed", &speed, .01f, 200.f))
			recalculateMaxStep();

		if (running)
		{
			if (ImGui::Button("Pause"))
				running = false;
		}
		else
		{
			if (ImGui::Button("Resume"))
			{
				running = true;
				clk.restart();
			}
		}
		ImGui::Checkbox("Recalculate", &recalculate);
		if (running)
		{
			dt += clk.restart().asSeconds();
			if (dt >= tickOff)
			{
				dt -= tickOff;
				tick();
			}
		}
	}
}

void PreviewVoronoi::onAdd()
{
	recalculateVoronoi();
}

void PreviewVoronoi::onDelete()
{
	recalculateVoronoi();
}

void PreviewVoronoi::onMove()
{
	recalculateVoronoi();
}

PreviewVoronoi::PreviewVoronoi(ViewerBase& viewerBase) :
	CommonEditor(viewerBase),
	lineColor(sf::Color::White),
	drawVoronoi(true),
	drawRobots(true),
	drawCenters(true),
	drawTargets(true),
	highlightHoveredRobot(true),
	highlightHoveredCell(false),
	mt(SEED),
	running(false),
	recalculate(true),
	speed(5.f),
	tickOff(1.f / DEFAULT_TICKS_PER_SECOND),
	dt(0)
{
	MakeUniquePtr(voronoiInfo, viewerBase);
	auto& circle = viewerBase.circle;
	circle.setOutlineColor(sf::Color::Green);
	recalculateMaxStep();
}

void PreviewVoronoi::tick()
{
	auto& coords = robotCoords;
	const auto& radii = robotRadius;
	const auto n = coords.size();
	std::vector<vec2d> vDeslocamento(n);
	for (size_t i = 0; i != n; i++)
	{
		if (i == circleMovingIndex)
			continue;

		const auto& target = voronoiInfo->targets[i];
		const auto& coord = coords[i];
		const auto& r1 = radii[i];
		auto& tot = vDeslocamento[i];

		if (auto off = target - coord) // se ainda não convergiu 100%
		{
			const auto lenLeft = length(off);
			const auto& walk = std::min(lenLeft, maxStep);
			tot += walk / lenLeft * off; // atração para o centro de gravidade da célula
		}

		// forças de repulsão
		static constexpr double D = 10; // raio de observação (além do próprio raio)
		for (size_t j = 0; j != n; j++)
		{
			if (j == i)
				continue;
			const auto& cOther = coords[j];
			if (auto dTot = coord - cOther) // repulsão em relação ao cOther
			{
				auto d = length(dTot);
				const auto sr = r1 + radii[j];
				if (d < D + sr) // outro agente dentro do nosso raio de observação
				{
					const auto actualD = d - sr;
					double f;
					if (actualD < invMaxStep) // já estão colidindo (actualD < 0) ou mto próximos q iam explodir de ir longe
					{
						//f = maxStep; // força máxima
						f = (2 - 2 / (1 + pow(std::numbers::e, -actualD))) * maxStep;
					}
					else
						f = 1 / actualD;
					const auto uDesloc = dTot / d;
					tot += f * uDesloc;
				}
			}
			else // dois agentes na mesma coordenada, corrigir a simulação com um deslocamento aleatório
			{
				std::uniform_real_distribution d(0., 2 * std::numbers::pi);
				auto angle = d(mt);
				vec2d rep{ cos(angle), sin(angle) };
				tot += rep * maxStep;
			}
		}
	}
	for (size_t i = 0; i != n; i++)
		coords[i] += vDeslocamento[i];
	if (recalculate)
		recalculateVoronoi();
}

void PreviewVoronoi::recalculateMaxStep()
{
	maxStep = static_cast<double>(tickOff * speed);
	invMaxStep = 1 / maxStep;
}