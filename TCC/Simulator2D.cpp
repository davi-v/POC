#include "Pch.hpp"
#include "Simulator2D.hpp"

#include "NavigatorRVO2.hpp"
#include "NavigatorCamposPotenciais.hpp"
#include "Hopcroft.hpp"
#include "Utilities.hpp"
#include "FileExplorer.hpp"
#include "Hungarian.hpp"
#include "Application.hpp"

bool Simulator2D::loadAgentsAndGoals(const std::wstring& path)
{
	if (auto f = std::ifstream(path, std::ios::binary | std::ios::ate))
	{
		auto retError = [&]
		{
			LOG("Error reading file ", path, '\n');
			return false;
		};

		size_t bytesLeft = f.tellg();
		auto nextSize = sizeof(size_t);
		if (bytesLeft < nextSize)
			return retError();

		f.seekg(0);

		size_t nAgents;
		FileRead(f, nAgents);
		bytesLeft -= nextSize;

		nextSize = nAgents * sizeof(vec2d);
		if (bytesLeft < nextSize)
			return retError();

		agents.clear();
		for (size_t i = 0; i != nAgents; i++)
		{
			vec2d coord;
			FileRead(f, coord);
			addAgent(coord);
		}
		bytesLeft -= nextSize;

		auto div = bytesLeft / sizeof(vec2d);
		auto mod = bytesLeft % sizeof(vec2d);
		if (mod != 0)
			return retError();
		if (nAgents == 0 && div == 0)
			return retError(); // não aceite arquivo sem nenhum vértice

		goals.clear();
		for (size_t i = 0; i != div; i++)
		{
			vec2d coord;
			FileRead(f, coord);
			addGoal(coord);
		}

		return true;
	}
	return false;
}

void Simulator2D::saveAgentsAndGoals(const std::wstring& path)
{
	if (auto f = std::ofstream(path, std::ios::binary))
	{
		auto nAgents = agents.size();
		FileAppend(f, nAgents);
		for (const auto& agent : agents)
			FileAppend(f, agent.coord);
		for (const auto& goal : goals)
			FileAppend(f, goal.coord);
	}
}

double Simulator2D::getDistanceSquaredToEdge(const SelectableEdge& e, const vec2d& c)
{
	auto d = cross(c - e.A, e.dir);
	return square(d);
}

vec2d Simulator2D::getCoordDouble(int x, int y)
{
	auto coordFloat = app.window.mapPixelToCoords({ x, y });
	vec2d coordDouble(static_cast<double>(coordFloat.x), static_cast<double>(coordFloat.y));
	return coordDouble;
}

void Simulator2D::addEdge(Agent& agent, Goal& goal)
{
	agent.par = &goal;
	goal.par = &agent;
}

Simulator2D::Simulator2D(Application* app, float radius) :
	app(*app),
	radius(radius),
	typeHovered(TypeHovered::None),
	usingOutline(false),
	metricBasedAllocation(true),
	curNavigator(NavigatorCheckbox::rvo2),
	addAction(AddAction::Agent),
	useDistancesSquared(true),
	distanceFunctionUsingType(DistanceFunctionsEnum::SumOfEachEdgeDistanceSquared),
	distanceFuncUsingCallback(DISTANCE_FUNCTIONS[static_cast<DistanceFunctionsUnderlyingType>(distanceFunctionUsingType)]),
	metric(Metric::MinMaxEdgeThenSum),
	currentMaxEdge(0),
	currentTotalSumOfEachEdgeDistance(0),
	currentTotalSumOfEachEdgeDistanceSquared(0),
	variance(0),
	colorEdge(sf::Color::White),
	colorEdgeMaxDistance(sf::Color::Yellow),
	goalColor(sf::Color::Green),
	defaultAgentColor(sf::Color::Blue)
{
	eventInterface = std::make_unique<AddAgentAction>(*this);
}

void Simulator2D::addAgent(const vec2d& c)
{
	agents.emplace_back(c, radius);
}

void Simulator2D::addAgent(float x, float y)
{
	addAgent(vec2d{ static_cast<double>(x), static_cast<double>(y) });
}

void Simulator2D::clearGoals()
{
	goals.clear();
}

void Simulator2D::addGoal(const vec2d& c)
{
	goals.emplace_back(c, radius);
}

void Simulator2D::addAgent(const sf::Vector2f& c)
{
	addAgent(c.x, c.y);
}

bool Simulator2D::drawUI()
{
	bool opened = true;
	ImGui::Begin("Editor", &opened);

	//ImGui::Text("Frame (ms): %.3f (%.1f FPS)", 1000 / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

	{
		std::string str;
		str += "Agents: " + std::to_string(agents.size());
		str += "\nGoals: " + std::to_string(goals.size());
		ImGui::Text(str.c_str());
	}

	if (ImGui::CollapsingHeader("Colors"))
	{
		ColorPicker3U32("Agent", defaultAgentColor);
		ColorPicker3U32("Goal", goalColor);
		ColorPicker3U32("Edge", colorEdge);
		ColorPicker3U32("Biggest Edge", colorEdgeMaxDistance);
	}

	if (ImGui::Checkbox("Outline Borders", &usingOutline))
	{
		if (usingOutline)
		{
			updateOutlineThickness();
			updateOutlineColor();
		}
		else
			circle.setOutlineThickness(0);
	}

	if (usingOutline)
	{
		if (ImGui::ColorPicker3("Outline Color", outlineColor, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha))
			updateOutlineColor();
		if (ImGui::SliderFloat("Outline Thickness Ratio", &percentageOutline, 0.f, 1.f))
			updateOutlineThickness();
	}

	if (!nav)
	{
		if (!agents.empty())
			if (ImGui::Button("Navigator"))
				tryCreateSelectedNavigator();
	}

	if (ImGui::Checkbox("Metric Based Allocation", &metricBasedAllocation))
		if (metricBasedAllocation)
			updateGraphEdges();

	if (!nav)
	{
		int amount;
		if (hasAtLeast1Edge())
			amount = 4; // also give option to add edge
		else
			amount = 3;
		static constexpr const char* PLACE_ACTION[] = {
		"Agent",
		"Goal",
		"Edge"
		};
		if (ImGui::Combo("Add", reinterpret_cast<int*>(&addAction), PLACE_ACTION, sizeof(PLACE_ACTION) / sizeof(*PLACE_ACTION)))
		{
			switch (addAction)
			{
			case AddAction::Agent:
			{
				eventInterface = std::make_unique<AddAgentAction>(*this);
			}
			break;
			case AddAction::Goal:
			{
				eventInterface = std::make_unique<AddGoalAction>(*this);
			}
			break;
			case AddAction::Edge:
			{
				eventInterface = std::make_unique<AddEdgeAction>(*this);
			}
			break;
			}
		}

		if (!agents.empty() || !goals.empty())
			if (ImGui::Button("Clear"))
				Clear(agents, goals);
	}

	if (metricBasedAllocation)
	{
		if (hasAtLeast1Edge() && ImGui::CollapsingHeader("Metrics"))
		{
			static constexpr const char* METRIC_MESSAGES[] = {
				"minimize total sum of edges",
				"minimize biggest edge",
				"minimize biggest edge then minimize total sum of edges"
			};
			if (ImGui::Combo("metric", reinterpret_cast<int*>(&metric), METRIC_MESSAGES, IM_ARRAYSIZE(METRIC_MESSAGES)))
				updateGraphEdges();

			// na métrica de apenas minimizar a maior aresta, não faz diferença se a distância considerada foi quadrática ou não
			if (metric != Metric::MinimizeBiggestEdge)
				if (ImGui::Checkbox("distances squared", &useDistancesSquared))
				{
					updateDistanceSquaredVars();
					updateGraphEdges(); // já sabemos que o grafo tem pelo menos 1 aresta
				}

			std::string str;
			str += "max edge: " + std::to_string(currentMaxEdge);
			str += "\nsum of edges: " + std::to_string(currentTotalSumOfEachEdgeDistance);
			str += "\nsum of each edge squared: " + std::to_string(currentTotalSumOfEachEdgeDistanceSquared);
			str += "\nvariance: " + std::to_string(variance);
			ImGui::Text(str.c_str());
		}
	}

	if (nav)
	{
		static constexpr const char* SIMULATION_LOGIC_LABELS[] = {
				"rvo2",
				"Campos Potenciais"
		};
		if (ImGui::Combo("Navigator", reinterpret_cast<int*>(&curNavigator), SIMULATION_LOGIC_LABELS, IM_ARRAYSIZE(SIMULATION_LOGIC_LABELS)))
			tryCreateSelectedNavigator();
		
		if (!nav->drawUI())
			nav.reset();

	}
	else
	{
		const auto prvRadius = radius;
		if (ImGui::SliderFloat("Radius", &radius, 0.5f, 100.f))
		{
			const auto m = radius / prvRadius;
			for (auto& agent : agents)
				agent.radius *= m;
		}

		if (canSaveFile() && ImGui::MenuItem("Save File"))
			saveFile();
		if (ImGui::MenuItem("Open File"))
			tryOpenFile();
	}

	ImGui::End();
	return opened;
}

void Simulator2D::draw()
{
	const auto dt = clock.restart().asSeconds();
	if (nav)
	{
		nav->loopUpdate();
		nav->draw();
	}
	else
	{
		if (typeHovered == TypeHovered::Agent)
		{
			static constexpr float RADIUS_CHANGE_SPEED_PER_SEC = DEFAULT_RADIUS * 2;
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
				elemHovered->radius += RADIUS_CHANGE_SPEED_PER_SEC * dt;
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
			{
				auto& r = elemHovered->radius -= RADIUS_CHANGE_SPEED_PER_SEC * dt;
				static constexpr float MIN_RADIUS = 1.f;
				if (r < MIN_RADIUS)
					r = MIN_RADIUS;
			}
		}

		for (const auto& agent : agents)
		{
			if (&agent == elemHovered)
				circle.setFillColor(sf::Color::Yellow);
			else
				circle.setFillColor(getColor(agent.coord));
			PrepareCircleRadius(circle, static_cast<float>(agent.radius));
			circle.setPosition(agent.coord);
			app.window.draw(circle);
		}
	
		for (const auto& goal : goals)
		{
			PrepareCircleRadius(circle, goal.radius);
			if (&goal == elemHovered)
				circle.setFillColor(sf::Color::Yellow);
			else
				circle.setFillColor(goalColor);
			circle.setPosition(goal.coord);
			app.window.draw(circle);
		}

		for (const auto& agent : agents)
		{
			if (const auto& goalPtr = agent.par)
			{
				const auto& goal = *goalPtr;
				sf::Color color;
				if (distance2(agent, goal) == currentMaxEdgeSquared)
					color = colorEdgeMaxDistance;
				else
					color = colorEdge;
				sf::Vertex vertices[2]
				{
					sf::Vertex{ agent.coord, color },
					sf::Vertex{ goal.coord, color },
				};
				app.window.draw(vertices, 2, sf::Lines);
			}
		}

	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Delete))
		if (typeHovered != TypeHovered::None)
		{
			const auto& it = elemHoveredIt;
			auto& e = *it;
			if (auto& p = e.par)
			{
				p->par = nullptr;
				p = nullptr;
			}
			if (typeHovered == TypeHovered::Agent)
				agents.erase(it);
			else
				goals.erase(it);
			tryUpdateAllocation();
			typeHovered = TypeHovered::None;
		}

	eventInterface->draw();
}

void Simulator2D::pollEvent(const sf::Event& event)
{
	if (NotHoveringIMGui())
		if (eventInterface)
			eventInterface->pollEvent(event);
}

void Simulator2D::clearAll()
{
	Clear(agents, goals);
	nav.reset();
}
void Simulator2D::recalculateElemHovered(const vec2d& coordDouble)
{
	typeHovered = TypeHovered::None;
	elemHovered = nullptr; // usado no draw
	if (auto opt = eventInterface->tryGetElementHovered(coordDouble))
	{
		std::tie(elemHoveredIt, typeHovered) = *opt;
		elemHovered = &*elemHoveredIt;
	}
}

void Simulator2D::updateOutlineColor()
{
	circle.setOutlineColor(ToSFMLColor(outlineColor));
}

void Simulator2D::updateOutlineThickness()
{
	circle.setOutlineThickness(-percentageOutline * radius);
}

bool Simulator2D::canSaveFile()
{
	return !agents.empty() || !goals.empty();
}

void Simulator2D::saveFile()
{
	if (auto selection = TryWriteFile(L"Select where to save the current agents and goals", 1, lFilterPatterns))
		saveAgentsAndGoals(selection);
}

void Simulator2D::tryOpenFile()
{
	if (auto selection = TryOpenFile(L"Select map to load", 1, lFilterPatterns))
		if (loadAgentsAndGoals(selection))
		{
			updateGraphEdges();

			// vamos colocar o centro da tela na média dos pontos
			double sX = 0, sY = 0;
			for (const auto& agent : agents)
			{
				sX += agent.coord.x;
				sY += agent.coord.y;
			}
			for (const auto& goal : goals)
			{
				sX += goal.coord.x;
				sY += goal.coord.y;
			}
			auto n = agents.size() + goals.size();
			sX /= n; // se n é 0, loadAgentsAndGoals retornou false e não estaríamos aqui
			sY /= n;
			sf::View view = app.window.getView();
			view.setCenter(static_cast<float>(sX), static_cast<float>(sY));

			app.zoomLevel = 0.f;
			app.zoomFac = 1.f;
			view.setSize(sf::Vector2f(app.window.getSize()) * app.zoomFac);
			app.window.setView(view);
		}
		else
			app.addMessage("Error reading file");
}

void Simulator2D::updateDistanceSquaredVars()
{
	distanceFunctionUsingType = useDistancesSquared ? DistanceFunctionsEnum::SumOfEachEdgeDistanceSquared : DistanceFunctionsEnum::SumOfEachEdgeDistance;
	distanceFuncUsingCallback = useDistancesSquared ? distance2 : distance1;
}

bool Simulator2D::hasAtLeast1Edge()
{
	auto nGoals = goals.size();
	return agents.size() >= nGoals && nGoals;
}

void Simulator2D::addGoalsAndRecalculate(const std::vector<sf::Vector2f>& goalsPositions)
{
	for (const auto& v : goalsPositions)
		addGoal({ static_cast<double>(v.x), static_cast<double>(v.y) });
	updateGraphEdges();
}

void Simulator2D::tryUpdateAllocation()
{
	if (metricBasedAllocation)
		updateGraphEdges();
}

sf::Color Simulator2D::getColor(float x, float y) const
{
	if (const auto& ptr = app.viewerBase)
		return ptr->getColor(x, y);
	return defaultAgentColor;
}

sf::Color Simulator2D::getColor(const sf::Vector2f& v) const
{
	return getColor(v.x, v.y);
}

void Simulator2D::tryCreateSelectedNavigator()
{
	for (auto& agent : agents)
		if (const auto& p = agent.par)
			p->radius = agent.radius;
	switch (curNavigator)
	{
	case NavigatorCheckbox::rvo2:
	{
		nav = std::make_unique<NavigatorRVO2>(*this);
	}
	break;
	case NavigatorCheckbox::CamposPotenciais:
	{
		nav = std::make_unique<NavigatorCamposPotenciais>(*this, radius * 6);
	}
	break;
	}
}

void Simulator2D::updateGraphEdges()
{
	for (auto& agent : agents)
		agent.par = nullptr;

	if (goals.empty() || agents.size() < goals.size())
		return;

	switch (metric)
	{
	case Metric::MinimizeTotalSumOfEdges:
	{
		const auto costMatrix = getHungarianCostMatrix();
		updateAgentsGoalsAndInternalVariablesWithHungarianAlgorithm(costMatrix, true);
	}
	break;
	case Metric::MinimizeBiggestEdge:
	{
		for (auto& agent : agents)
			agent.par = nullptr;
		auto edges = updateInternalVariablesMetricEdgesMinimizingBiggestEdge();
		for (const auto& [agentIdx, goalIdx] : edges)
		{
			auto itAgent = agents.begin();
			auto itGoal = goals.begin();
			std::advance(itAgent, agentIdx);
			std::advance(itGoal, goalIdx);
			addEdge(*itAgent, *itGoal);
		}
	}
	break;
	case Metric::MinMaxEdgeThenSum:
	{
		updateInternalVariablesMetricEdgesMinimizingBiggestEdge();

		double limCost;
		if (distanceFunctionUsingType == DistanceFunctionsEnum::SumOfEachEdgeDistance)
			limCost = currentMaxEdge;
		else
			limCost = currentMaxEdgeSquared;

		auto costMatrix = getHungarianCostMatrix();

		// sabemos que existe solução com arestas com peso no máximo limCost
		// vamos marcar custo infinito nas arestas mais pesadas que limCost para serem ignoradas pelo algoritmo hungariano
		for (auto& row : costMatrix)
			for (auto& elem : row)
				if (elem > limCost)
					elem = std::numeric_limits<double>::max(); // o algoritmo hungariano opera diminuindo esse valor, então não ocorre overflow

		updateAgentsGoalsAndInternalVariablesWithHungarianAlgorithm(costMatrix, false);
	}
	break;
	}
}

std::vector<vec2d> Simulator2D::getAgentsCoordinatesFromNavigator()
{
	return nav->getAgentPositions();
}

void Simulator2D::recreateNavigatorAndPlay()
{
	tryCreateSelectedNavigator();
	nav->startNavigating();
}

void Simulator2D::updateAgentsGoalsAndInternalVariablesWithHungarianAlgorithm(const CostMatrix& costMatrix, bool needsToUpdateMaxEdge)
{
	Hungarian<double> h(costMatrix);
	auto [cost, assignment] = h.assignment();

	std::vector<double> distances;

	if (distanceFunctionUsingType == DistanceFunctionsEnum::SumOfEachEdgeDistance)
	{
		currentTotalSumOfEachEdgeDistance = cost;
		currentTotalSumOfEachEdgeDistanceSquared = 0;
	}
	else
	{
		currentTotalSumOfEachEdgeDistance = 0;
		currentTotalSumOfEachEdgeDistanceSquared = cost;
	}

	if (needsToUpdateMaxEdge)
		currentMaxEdge = 0;

	auto
		nAgents = agents.size(),
		nGoals = goals.size();

	auto fillIts = [&](auto& c)
	{
		const auto n = c.size();
		std::vector<std::list<ElementInteractable>::iterator> ret(n);
		auto it = c.begin();
		for (size_t i = 0; i != n; )
			ret[i++] = it++;
		return ret;
	};
	auto agentsIts = fillIts(agents);
	auto goalsIts = fillIts(goals);

	for (size_t agentIndex = 0; agentIndex != nAgents; agentIndex++)
	{
		auto& agent = *agentsIts[agentIndex];
		auto& goalIndex = assignment[agentIndex];
		if (goalIndex < nGoals) // agente foi escolhido para ir a algum goal
		{
			auto& goal = *goalsIts[goalIndex];
			addEdge(agent, goal);
			const auto& costInMatrix = costMatrix[agentIndex][goalIndex];
			double edgeCostSquared;
			double edgeCost;
			if (useDistancesSquared)
			{
				edgeCost = sqrt(costInMatrix);
				edgeCostSquared = costInMatrix;
				currentTotalSumOfEachEdgeDistance += edgeCost;
			}
			else
			{
				edgeCost = costInMatrix;
				edgeCostSquared = distance2(agent, goal); // temos que calcular do mesmo jeito em todos os lugares para evitar erros de ponto flutuante. costInMatrix foi calculado usando sqrt. Fazer ao quadrado poder dar resultado diferente de fazer usando o dot product que já sai ao quadrado
				currentTotalSumOfEachEdgeDistanceSquared += edgeCostSquared;
			}
			distances.emplace_back(edgeCost);

			// here, edgeCost is the cost of the edge not squared
			if (needsToUpdateMaxEdge)
			{
				if (edgeCost > currentMaxEdge)
				{
					currentMaxEdge = edgeCost;
					currentMaxEdgeSquared = edgeCostSquared;
				}
			}
		}
		else
			agent.par = nullptr;
	}

	variance = 0;
	const auto mean = currentTotalSumOfEachEdgeDistance / nGoals;
	for (const auto& d : distances)
		variance += square(d - mean);
	variance /= nGoals;
}

CostMatrix Simulator2D::getHungarianCostMatrix() const
{
	auto
		nAgents = agents.size(),
		nGoals = goals.size();
	auto costMatrix = std::vector(nAgents, std::vector<double>(nAgents));
	auto itAgent = agents.begin();
	for (size_t agentIndex = 0; agentIndex != nAgents; agentIndex++)
	{
		auto& agent = *itAgent++;
		auto itGoal = goals.begin();
		for (size_t goalIndex = 0; goalIndex != nGoals; goalIndex++)
			costMatrix[agentIndex][goalIndex] = distanceFuncUsingCallback(agent, *itGoal++);
		for (size_t goalIndex = nGoals; goalIndex != nAgents; goalIndex++)
			costMatrix[agentIndex][goalIndex] = 0;
	}

	return costMatrix;
}

Edges Simulator2D::updateInternalVariablesMetricEdgesMinimizingBiggestEdge()
{
	auto
		nAgents = agents.size(),
		nGoals = goals.size();

	auto costMatrix = std::vector<std::vector<double>>(nAgents, std::vector<double>(nGoals));
	std::vector<double> sortedEdgeCosts(nAgents * nGoals);
	auto agentPtr = agents.begin();
	size_t curOff = 0;
	for (size_t agentIndex = 0; agentIndex != nAgents; agentIndex++, agentPtr++)
	{
		auto goalPtr = goals.begin();
		for (size_t goalIndex = 0; goalIndex != nGoals; goalIndex++, goalPtr++)
		{
			// minimizar a maior distância / minimizar a maior distância ao quadrado dá na mesma. Mas o último é mais barato
			auto& v = costMatrix[agentIndex][goalIndex] = distance2(*agentPtr, *goalPtr);
			sortedEdgeCosts[curOff++] = v;
		}
	}
	auto beg = sortedEdgeCosts.begin();
	auto end = sortedEdgeCosts.end();
	std::sort(beg, end);
	size_t l = 0, r = std::distance(beg, std::unique(beg, end));
	Edges edgesOfBestMatching;
	while (l != r)
	{
		auto m = (l + r) / 2;
		const auto& curCostEvaluating = sortedEdgeCosts[m];
		bipartite_matching matching{ nAgents, nGoals };
		for (size_t agentIndex = 0; agentIndex != nAgents; agentIndex++)
			for (size_t goalIndex = 0; goalIndex != nGoals; goalIndex++)
				if (costMatrix[agentIndex][goalIndex] <= curCostEvaluating)
					matching.add(agentIndex, goalIndex);
		auto isPossible = matching.get_max_matching() == nGoals;
		if (isPossible)
		{
			r = m;
			edgesOfBestMatching = matching.get_edges();
			currentMaxEdgeSquared = curCostEvaluating;
		}
		else
			l = m + 1;
	}

	currentMaxEdge = sqrt(currentMaxEdgeSquared);

	std::vector<double> distances;

	currentTotalSumOfEachEdgeDistanceSquared = currentTotalSumOfEachEdgeDistance = 0;
	for (const auto& [agentIndex, goalIndex] : edgesOfBestMatching)
	{
		const auto& costSquared = costMatrix[agentIndex][goalIndex];
		currentTotalSumOfEachEdgeDistanceSquared += costSquared;

		const auto d = sqrt(costSquared);
		distances.emplace_back(d);
		currentTotalSumOfEachEdgeDistance += d;
	}

	variance = 0;
	auto mean = currentTotalSumOfEachEdgeDistance / nGoals;
	for (const auto& d : distances)
		variance += square(d - mean);
	variance /= nGoals;

	return edgesOfBestMatching;
}

Simulator2D::AddAgentAction::AddAgentAction(Simulator2D& sim) :
	AddItemAction(sim)
{
}

void Simulator2D::AddAgentAction::placeItem(double x, double y)
{
	sim.addAgent((float)x, (float)y);
}

void Simulator2D::AddGoalAction::placeItem(double x, double y)
{
	sim.addGoal({ x, y });
}

Simulator2D::AddGoalAction::AddGoalAction(Simulator2D& sim) :
	AddItemAction(sim)
{
}

std::optional<std::pair<Simulator2D::EventInterface::It, Simulator2D::TypeHovered>>
	Simulator2D::EventInterface::tryGetElementHovered(
		const vec2d& coordDouble)
{
	bool found = false;
	It retIt;
	auto curMinD2 = std::numeric_limits<float>::max();
	TypeHovered typeHovered;
	auto helper = [&](auto&& c, TypeHovered type)
	{
		auto it = c.begin(), lim = c.end();
		while (it != lim)
		{
			auto& e = *it;
			if (TryUpdateClosestCircle(coordDouble, curMinD2, e))
			{
				found = true;
				retIt = it;
				typeHovered = type;
			}
			it++;
		}
	};
	helper(sim.agents, TypeHovered::Agent);
	helper(sim.goals, TypeHovered::Goal);
	if (found)
		return { {retIt, typeHovered} };
	return {};
}

Simulator2D::EventInterface::EventInterface(Simulator2D& sim) :
	sim(sim),
	isHoldingLeftClick(false),
	isHoldingRightClick(false)
{
}

void Simulator2D::EventInterface::draw()
{
}

void Simulator2D::EventInterface::pollEvent(const sf::Event& event)
{
	if (sim.nav)
		return;
	switch (event.type)
	{
	case sf::Event::MouseButtonPressed:
	{
		switch (event.mouseButton.button)
		{
		case sf::Mouse::Left:
		{
			isHoldingLeftClick = true;
		}
		break;
		case sf::Mouse::Right:
		{
			isHoldingRightClick = true;
			if (sim.typeHovered != TypeHovered::None)
			{
				const auto& coord = sim.elemHovered->coord;
				const auto coordDouble = sim.getCoordDouble(event.mouseButton.x, event.mouseButton.y);
				curOff = coord - coordDouble;
			}
		}
		break;
		}
	}
	break;
	case sf::Event::MouseButtonReleased:
	{
		switch (event.mouseButton.button)
		{
		case sf::Mouse::Left:
		{
			isHoldingLeftClick = false;
		}
		break;
		case sf::Mouse::Right:
		{
			isHoldingRightClick = false;
		}
		break;
		}
	}
	break;
	case sf::Event::MouseMoved:
	{
		const auto coord = sim.getCoordDouble(
			event.mouseMove.x,
			event.mouseMove.y
		);
		if (isHoldingRightClick && sim.typeHovered != TypeHovered::None) // move agent
		{
			sim.elemHovered->coord = coord + curOff;
			sim.tryUpdateAllocation();
		}
		else
			sim.recalculateElemHovered(coord);
	}
	break;
	}
	pollEventExtra(event);
}

void Simulator2D::AddItemAction::pollEventExtra(const sf::Event& event)
{
	switch (event.type)
	{
	case sf::Event::MouseButtonPressed:
	{
		switch (event.mouseButton.button)
		{
		case sf::Mouse::Left:
		{
			didNotMoveSinceLastPress = true;
		}
		break;
		}
	}
	break;
	case sf::Event::MouseButtonReleased:
	{
		switch (event.mouseButton.button)
		{
		case sf::Mouse::Left:
		{
			if (didNotMoveSinceLastPress)
			{
				auto& x = event.mouseButton.x;
				auto& y = event.mouseButton.y;
				auto coord = sim.getCoordDouble(x, y);
				placeItem(coord.x, coord.y);
				sim.tryUpdateAllocation();
				sim.recalculateElemHovered({ coord.x, coord.y });
			}
		}
		break;
		}
	}
	break;
	case sf::Event::MouseMoved:
	{
		didNotMoveSinceLastPress = false;
	}
	break;
	}
}

Simulator2D::AddItemAction::AddItemAction(Simulator2D& sim) :
	EventInterface(sim),
	didNotMoveSinceLastPress(true)
{
}

void Simulator2D::AddEdgeAction::clear()
{
	curAgent = nullptr;
	curGoal = nullptr;
}

void Simulator2D::AddEdgeAction::pollEventExtra(const sf::Event& event)
{
	switch (event.type)
	{
	case sf::Event::MouseButtonPressed:
	{
		switch (event.mouseButton.button)
		{
		case sf::Mouse::Left:
		{
			const auto& typeHovered = sim.typeHovered;
			if (typeHovered != TypeHovered::None)
			{
				elemSelected = sim.elemHovered;
				if (typeHovered == TypeHovered::Agent)
					curAgent = static_cast<Agent*>(elemSelected);
				else
					curGoal = static_cast<Goal*>(elemSelected);
			}
			else
				clear();

			if (curAgent && curGoal)
			{
				auto tryDisconnect = [&](auto& x)
				{
					if (auto& p = x->par)
						p->par = nullptr;
				};
				tryDisconnect(curAgent);
				tryDisconnect(curGoal);
				sim.addEdge(*curAgent, *curGoal);
				clear();
			}
		}
		break;
		}
	}
	break;
	}
}

Simulator2D::AddEdgeAction::AddEdgeAction(Simulator2D& sim) :
	EventInterface(sim),
	elemSelected(nullptr),
	curAgent(nullptr),
	curGoal(nullptr)
{
}

void Simulator2D::AddEdgeAction::draw()
{
	auto overlayCircleWithColor = [&](ElementInteractable* ePtr, const sf::Color& color)
	{
		const auto& coord = ePtr->coord;
		const auto& r = ePtr->radius;
		auto& c = sim.circle;
		c.setPosition(coord);
		c.setFillColor(color);
		PrepareCircleRadius(c, r);
		sim.app.window.draw(c);
	};

	static const sf::Color SELECTING_VERTEX_FOR_NEW_ADD_COLOR = sf::Color(0xff9933ef);
	if (curAgent)
		overlayCircleWithColor(curAgent, SELECTING_VERTEX_FOR_NEW_ADD_COLOR);
	if (curGoal)
		overlayCircleWithColor(curGoal, SELECTING_VERTEX_FOR_NEW_ADD_COLOR);
}

void Simulator2D::quitNavigator()
{
	auto agentsCoords = getAgentsCoordinatesFromNavigator();
	nav.reset();
	auto n = agentsCoords.size();
	size_t i = 0;
	auto it = agents.begin(), end = agents.end();
	while (it != end)
	{
		it->coord = agentsCoords[i];
		it++; i++;
	}
}

double distance1(const ElementInteractable& e1, const ElementInteractable& e2)
{
	return distance(e1.coord, e2.coord);
}

double distance2(const ElementInteractable& e1, const ElementInteractable& e2)
{
	return distanceSquared(e1.coord, e2.coord);
}
