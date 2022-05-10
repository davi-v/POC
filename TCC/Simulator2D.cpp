#include "Pch.hpp"
#include "Simulator2D.hpp"

#include "DrawUtil.hpp"
#include "NavigatorRVO2.hpp"
#include "NavigatorCamposPotenciais.hpp"
#include "Hopcroft.hpp"
#include "Utilities.hpp"

const sf::Color Simulator2D::DEFAULT_GOAL_COLOR = sf::Color::Green;

void Simulator2D::addMessage(const char* msg)
{
	warnings.emplace_back(msg, globalClock.getElapsedTime());
}

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
		agents.resize(nAgents);
		for (size_t i = 0; i != nAgents; i++)
		{
			vec2d coord;
			FileRead(f, coord);
			auto& agent = agents[i];
			agent = std::make_unique<Agent2D>(coord);
		}
		bytesLeft -= nextSize;

		auto div = bytesLeft / sizeof(vec2d);
		auto mod = bytesLeft % sizeof(vec2d);
		if (mod != 0)
			return retError();
		if (nAgents == 0 && div == 0)
			return retError(); // não aceite arquivo sem nenhum vértice
		goals.resize(div);
		for (size_t i = 0; i != div; i++)
		{
			vec2d coord;
			FileRead(f, coord);
			auto& goal = goals[i];
			goal = std::make_unique<Goal>(coord);
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
		for (const auto& agent : agents) FileAppend(f, agent->coord);
		for (const auto& goal : goals) FileAppend(f, goal->coord);
	}
}

double Simulator2D::getDistanceSquaredToEdge(const SelectableEdge& e, const vec2d& c)
{
	auto d = cross(c - e.A, e.dir);
	return square(d);
}

vec2d Simulator2D::getCoordDouble(int x, int y)
{
	auto coordFloat = window.mapPixelToCoords({ x, y });
	vec2d coordDouble(static_cast<double>(coordFloat.x), static_cast<double>(coordFloat.y));
	return coordDouble;
}

void Simulator2D::addEdge(Agent2D& agent, const Goal& goal)
{
	agent.updateGoal(goal);
}

void Simulator2D::updateZoomFacAndViewSizeFromZoomLevel(sf::View& view)
{
	zoomFac = powf(2, static_cast<float>(zoomLevel) / N_SCROLS_TO_DOUBLE);
	view.setSize(sf::Vector2f{ window.getSize() } * zoomFac);
}

Simulator2D::Simulator2D(sf::RenderWindow& window) :
	editModeType(EditModeType::Free),
	usingOutline(false),
	curNavigator(NavigatorCheckbox::rvo2),
	leftClickAction(LeftClickAction::Select),
	tickRate(DEFAULT_TICKS_PER_SECOND),
	curTimeStep(DEFAULT_TIME_STEP),
	distanceFunctionUsingType(DistanceFunctionsEnum::SumOfEachEdgeDistance),
	distanceFuncUsingCallback(DISTANCE_FUNCTIONS[static_cast<DistanceFunctionsUnderlyingType>(distanceFunctionUsingType)]),
	window(window),
	metric(Metric::MinimizeTotalSumOfEdges),
	zoomLevel(DEFAULT_ZOOM_LEVEL),
	lastPxClickedX(-1),
	zoomFac(1),
	currentMaxEdge(0),
	currentTotalSumOfEachEdgeDistance(0),
	currentTotalSumOfEachEdgeDistanceSquared(0)
{
	font.loadFromFile("segoeui.ttf");

	circle.setOutlineColor(sf::Color::White);

	textPopUpMessages.setFont(font);
	textPopUpMessages.setFillColor(sf::Color::Red);

	leftClickInterface = std::make_unique<SelectAction>(*this);
}

void Simulator2D::addAgent(const Agent2D& agent)
{
	agents.emplace_back(std::make_unique<Agent2D>(agent));
}

void Simulator2D::addGoal(const Goal& goal)
{
	goals.emplace_back(std::make_unique<Goal>(goal));
}

static void HelpMarker(const char* desc)
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

void Simulator2D::draw()
{
	ImGui::Begin("Settings");
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

	{
		std::string str;
		str += "Agents: " + std::to_string(agents.size());
		str += "\nGoals: " + std::to_string(goals.size());
		ImGui::Text(str.c_str());
	}

	if (ImGui::Checkbox("Outline Borders", &usingOutline))
	{
		if (usingOutline)
			circle.setOutlineThickness(-1);
		else
			circle.setOutlineThickness(0);
	}

	static constexpr const char* MODE_LABELS[] = {
		"Free Edit",
		"Metric Based",
		"Navigator"
	};
	if (ImGui::Combo("Mode", reinterpret_cast<int*>(&editModeType), MODE_LABELS, IM_ARRAYSIZE(MODE_LABELS)))
	{
		switch (editModeType)
		{
		case EditModeType::Free:
		{
		}
		break;
		case EditModeType::Metric:
		{
			updateGraphEdges();
			if (leftClickAction == LeftClickAction::AddEdge)
				leftClickAction = LeftClickAction::Select;
		}
		break;
		case EditModeType::Navigation:
		{
			createSelectedNavigator();
		}
		break;
		}
	}

	auto drawEditMenu = [&](int nTypes)
	{
		static constexpr const char* LEFT_CLICK_ACTION[] = {
		"Select",
		"Place Agent",
		"Place Goal",
		"Add Edge"
		};
		if (ImGui::Combo("Left Click Action", reinterpret_cast<int*>(&leftClickAction), LEFT_CLICK_ACTION, nTypes))
		{
			switch (leftClickAction)
			{
			case LeftClickAction::Select:
			{
				leftClickInterface = std::make_unique<SelectAction>(*this);
			}
			break;
			case LeftClickAction::PlaceAgent:
			{
				leftClickInterface = std::make_unique<AddAgentAction>(*this);
			}
			break;
			case LeftClickAction::PlaceGoal:
			{
				leftClickInterface = std::make_unique<AddGoalAction>(*this);
			}
			break;
			case LeftClickAction::AddEdge:
			{
				leftClickInterface = std::make_unique<AddEdgeAction>(*this);
			}
			break;
			}
		}

		if (ImGui::Button("Clear"))
			Clear(agents, goals);

		leftClickInterface->draw();
	};

	auto showFileActions = [&]
	{
		if (canSaveFile() && ImGui::Button("Save File"))
			saveFile();
		if (ImGui::Button("Open File"))
			tryOpenFile();
	};

	switch (editModeType)
	{
	case EditModeType::Free:
	{
		int amount;
		if (hasAtLeast1Edge())
			amount = 4;
		else
			amount = 3;
		drawEditMenu(amount);

		showFileActions();
	}
	break;
	case EditModeType::Metric:
	{
		drawEditMenu(3);
		if (hasAtLeast1Edge() && ImGui::CollapsingHeader("Metrics"))
		{
			static constexpr const char* METRIC_MESSAGES[] = {
				"minimize biggest edge",
				"minimize total sum of edges",
				"minimize biggest edge then minimize total sum of edges"
			};
			if (ImGui::Combo("metric", reinterpret_cast<int*>(&metric), METRIC_MESSAGES, IM_ARRAYSIZE(METRIC_MESSAGES)))
				updateGraphEdges();

			// na métrica de apenas minimizar a maior aresta, não faz diferença se a distância considerada foi quadrática ou não
			if (metric != Metric::MinimizeBiggestEdge)
				if (ImGui::Checkbox("distances squared", &usingDistancesSquared))
				{
					updateDistanceSquaredVars();
					updateGraphEdges(); // já sabemos que o grafo tem pelo menos 1 aresta
				}

			std::string str;
			str += "max edge: " + std::to_string(currentMaxEdge);
			str += "\nsum of edges: " + std::to_string(currentTotalSumOfEachEdgeDistance);
			str += "\nsum of each edge squared: " + std::to_string(currentTotalSumOfEachEdgeDistanceSquared);
			ImGui::Text(str.c_str());
		}

		showFileActions();
	}
	break;
	case EditModeType::Navigation:
	{
		static constexpr const char* SIMULATION_LOGIC_LABELS[] = {
		"rvo2",
		"Campos Potenciais"
		};
		if (ImGui::Combo("Navigator", reinterpret_cast<int*>(&curNavigator), SIMULATION_LOGIC_LABELS, IM_ARRAYSIZE(SIMULATION_LOGIC_LABELS)))
			createSelectedNavigator();

		if (ImGui::DragInt("Tick Rate", &tickRate, 1.0f, 1, 256, "%d", ImGuiSliderFlags_AlwaysClamp))
		{
			curTimeStep = 1.f / tickRate;
			navigator->updateTimeStep(curTimeStep);
		}
		ImGui::SameLine(); HelpMarker("How many ticks per second to run the simulation. Since it's run in the same thread as the rendering thread, it's clipped by the framerate");

		if (navigator->running)
		{
			auto curTime = globalClock.getElapsedTime().asSeconds();
			if (curTime - navigatorLastTick >= curTimeStep)
			{
				navigatorLastTick = curTime;
				static sf::Clock c;
				navigator->tick();
				LOG(1./c.restart().asSeconds(), '\n');
			}

			if (ImGui::Button("Pause"))
				navigator->running = false;
		}
		else
		{
			if (ImGui::Button("Play"))
			{
				navigator->running = true;
				navigatorLastTick = globalClock.getElapsedTime().asSeconds();
			}
		}
		ImGui::Checkbox("Draw Destination Lines", &navigator->drawDestinationLines);
		navigator->draw();
	}
	break;
	}

	ImGui::End(); // Settings

	// draw UI in screen space
	sf::View curView = window.getView(); // to restore later
	sf::View orgView = window.getDefaultView();
	window.setView(orgView);

	// pop-up messages
	std::string str;
	for (auto it = warnings.begin(); it != warnings.end(); )
	{
		auto& msg = it->first;
		auto& ts = it->second;
		auto now = globalClock.getElapsedTime();
		if ((now - ts).asSeconds() > WARNING_DURATION)
			it = warnings.erase(it);
		else
		{
			str += msg;
			str += '\n';
			it++;
		}
	}
	textPopUpMessages.setString(str);
	auto bounds = textPopUpMessages.getLocalBounds();
	textPopUpMessages.setOrigin(bounds.width * 0.5f, 0);
	auto [w, h] = window.getSize();
	textPopUpMessages.setPosition(w * 0.5f, 0);
	window.draw(textPopUpMessages);

	window.setView(curView); // restore current view
}

void Simulator2D::pollEvent(const sf::Event& event)
{
	if (notHoveringUI())
	{
		if (editModeType != EditModeType::Navigation)
			leftClickInterface->pollEvent(event);

		switch (event.type)
		{
		case sf::Event::MouseButtonPressed:
		{
			auto& data = event.mouseButton;
			switch (data.button)
			{
			case sf::Mouse::Left:
			{
				lastPxClickedX = data.x;
				lastPxClickedY = data.y;
			}
			break;
			}
		}
		break;
		}
	}

	switch (event.type)
	{
	case sf::Event::MouseWheelScrolled:
	{
		auto& scrollData = event.mouseWheelScroll;
		zoomLevel -= scrollData.delta;

		auto& px = scrollData.x;
		auto& py = scrollData.y;

		auto pixelCoord = sf::Vector2i{ px, py };

		sf::View view = window.getView();
		auto prevCoord = window.mapPixelToCoords(pixelCoord, view);
		updateZoomFacAndViewSizeFromZoomLevel(view);
		auto newCoord = window.mapPixelToCoords(pixelCoord, view);
		view.move(prevCoord - newCoord);

		window.setView(view);
	}
	break;
	case sf::Event::MouseMoved:
	{
		if (leftClickInterface->canMoveView())
			if (lastPxClickedX != -1) // if we are holding left click, move view of the world
			{
				auto& data = event.mouseMove;
				auto& newX = data.x;
				auto& newY = data.y;

				sf::View view = window.getView();
				view.move((lastPxClickedX - newX) * zoomFac, (lastPxClickedY - newY) * zoomFac);
				window.setView(view);

				lastPxClickedX = newX;
				lastPxClickedY = newY;
			}
	}
	break;
	case sf::Event::MouseButtonReleased:
	{
		switch (event.mouseButton.button)
		{
		case sf::Mouse::Left:
		{
			lastPxClickedX = -1; // no longer moving the scene
		}
		break;
		}
	}
	break;
	}
}

bool Simulator2D::canSaveFile()
{
	return !agents.empty() || !goals.empty();
}

void Simulator2D::saveFile()
{
	if (auto selection = tinyfd_saveFileDialogW( // there is also a wchar_t version
		L"Select where to save the current agents and goals", // title
		nullptr, // optional initial directory
		1, // number of filter patterns
		lFilterPatterns, // char const * lFilterPatterns[2] = { "*.txt", "*.jpg" };
		nullptr
	))
		saveAgentsAndGoals(selection);
}

void Simulator2D::tryOpenFile()
{
	if (auto selection = tinyfd_openFileDialogW( // there is also a wchar_t version
		L"Select map to load", // title
		nullptr, // optional initial directory
		1, // number of filter patterns
		lFilterPatterns, // char const * lFilterPatterns[2] = { "*.txt", "*.jpg" };
		NULL, // optional filter description
		0 // forbid multiple selections
	))
		if (loadAgentsAndGoals(selection))
		{
			updateGraphEdges();

			// vamos colocar o centro da tela na média dos pontos
			double sX = 0, sY = 0;
			for (const auto& agent : agents)
			{
				sX += agent->coord.x;
				sY += agent->coord.y;
			}
			for (const auto& goals : goals)
			{
				sX += goals->coord.x;
				sY += goals->coord.y;
			}
			auto n = agents.size() + goals.size();
			sX /= n; // se n é 0, loadAgentsAndGoals retornou false e não estaríamos aqui
			sY /= n;
			sf::View view = window.getView();
			view.setCenter(static_cast<float>(sX), static_cast<float>(sY));
			zoomLevel = 0;
			updateZoomFacAndViewSizeFromZoomLevel(view);
			window.setView(view);
		}
		else
			addMessage("Error reading file");
}

bool Simulator2D::notHoveringUI()
{
	return !ImGui::IsWindowHovered(
		ImGuiHoveredFlags_AnyWindow |
		//ImGuiHoveredFlags_DockHierarchy |
		ImGuiHoveredFlags_AllowWhenBlockedByPopup |
		ImGuiHoveredFlags_AllowWhenBlockedByActiveItem
	);
}

void Simulator2D::defaultDraw()
{
	for (const auto& agent : agents)
	{
		circle.setFillColor(agent->color);
		PrepareCircleRadius(circle, static_cast<float>(agent->radius));

		const auto& coord = agent->coord;
		circle.setPosition(coord);
		window.draw(circle);

		if (agent->goalPtr)
		{
			const auto& goal = *agent->goalPtr;

			sf::Color color;
			if (distanceSquaredAgent(*agent, goal) == currentMaxEdgeSquared)
				color = sf::Color::Yellow;
			else
				color = sf::Color::White;
			sf::Vertex vertices[2]
			{
				sf::Vertex{ agent->coord, color },
				sf::Vertex{ goal.coord, color },
			};
			window.draw(vertices, 2, sf::Lines);
		}
	}

	circle.setFillColor(DEFAULT_GOAL_COLOR);
	PrepareCircleRadius(circle, DEFAULT_GOAL_RADIUS);
	for (const auto& goal : goals)
	{
		circle.setPosition(goal->coord);
		window.draw(circle);
	}
}

void Simulator2D::updateDistanceSquaredVars()
{
	distanceFunctionUsingType = usingDistancesSquared ? DistanceFunctionsEnum::SumOfEachEdgeDistanceSquared : DistanceFunctionsEnum::SumOfEachEdgeDistance;
	distanceFuncUsingCallback = usingDistancesSquared ? distanceSquaredAgent : distanceAgent;
}

bool Simulator2D::hasAtLeast1Edge()
{
	auto nGoals = goals.size();
	return agents.size() >= nGoals && nGoals;
}

void Simulator2D::createSelectedNavigator()
{
	switch (curNavigator)
	{
	case NavigatorCheckbox::rvo2:
	{
		navigator = std::make_unique<NavigatorRVO2>(*this);
	}
	break;
	case NavigatorCheckbox::CamposPotenciais:
	{
		navigator = std::make_unique<NavigatorCamposPotenciais>(*this);
	}
	break;
	}
	for (const auto& agent : agents)
		navigator->addAgent(*agent);
}

//bool Simulator2D::reachedGoal()
//{
//	/* Check if all agents have reached their goals. */
//	for (size_t i = 0, end = agents.size(); i != end; i++)
//		if (RVO::absSq(sim.getAgentPosition(i) - goals[i]) > sim.getAgentRadius(i) * sim.getAgentRadius(i))
//			return false;
//	return true;
//}

void Simulator2D::updateGraphEdges()
{
	for (auto& agent : agents)
		agent->goalPtr = nullptr;

	if (goals.empty() || agents.size() < goals.size())
		return;

	switch (metric)
	{
	case Metric::MinimizeTotalSumOfEdges:
	{
		auto costMatrix = getHungarianCostMatrix();
		updateAgentsGoalsAndInternalVariablesWithHungarianAlgorithm(costMatrix, true);
	}
	break;
	case Metric::MinimizeBiggestEdge:
	{
		for (auto& agent : agents)
			agent->goalPtr = nullptr;
		for (auto& [agentIndex, goalIndex] : getEdgesMinimizingBiggestEdgeAndUpdateInternalVariables())
			agents[agentIndex]->updateGoal(*goals[goalIndex]);
	}
	break;
	case Metric::MinMaxEdgeThenSum:
	{
		getEdgesMinimizingBiggestEdgeAndUpdateInternalVariables();

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

void Simulator2D::updateAgentsGoalsAndInternalVariablesWithHungarianAlgorithm(CostMatrix& costMatrix, bool needsToUpdateMaxEdge)
{
	HungarianAlgorithm HungAlgo;
	std::vector<int> assignment;
	auto cost = HungAlgo.Solve(costMatrix, assignment);
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

	for (size_t agentIndex = 0; agentIndex != nAgents; agentIndex++)
	{
		auto& agent = agents[agentIndex];
		auto& goalIndex = assignment[agentIndex];
		if (goalIndex < nGoals) // agente foi escolhido para ir a algum goal
		{
			agent->updateGoal(*goals[goalIndex]);
			auto edgeCost = costMatrix[agentIndex][goalIndex];
			double edgeCostSquared;
			if (distanceFunctionUsingType == DistanceFunctionsEnum::SumOfEachEdgeDistance)
			{
				edgeCostSquared = distanceSquaredAgent(*agents[agentIndex], *goals[goalIndex]); // temos que calcular do mesmo jeito em todos os lugares para evitar erros de ponto flutuante
				currentTotalSumOfEachEdgeDistanceSquared += edgeCostSquared;
			}
			else
			{
				edgeCostSquared = edgeCost;
				edgeCost = sqrt(edgeCost);

				currentTotalSumOfEachEdgeDistance += edgeCost;
			}

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
			agent->goalPtr = nullptr;
	}
}

CostMatrix Simulator2D::getHungarianCostMatrix()
{
	auto
		nAgents = agents.size(),
		nGoals = goals.size();

	auto costMatrix = std::vector<std::vector<double>>(nAgents, std::vector<double>(nAgents));
	for (size_t agentIndex = 0; agentIndex != nAgents; agentIndex++)
	{
		auto& agent = agents[agentIndex];
		for (size_t goalIndex = 0; goalIndex != nGoals; goalIndex++)
			costMatrix[agentIndex][goalIndex] = distanceFuncUsingCallback(*agent, *goals[goalIndex]);
		for (size_t goalIndex = nGoals; goalIndex != nAgents; goalIndex++)
			costMatrix[agentIndex][goalIndex] = 0;
	}

	return costMatrix;
}

Edges Simulator2D::getEdgesMinimizingBiggestEdgeAndUpdateInternalVariables()
{
	auto
		nAgents = agents.size(),
		nGoals = goals.size();

	auto costMatrix = std::vector<std::vector<double>>(nAgents, std::vector<double>(nGoals));
	std::set<double> uniqueEdgeCosts;
	for (size_t agentIndex = 0; agentIndex != nAgents; agentIndex++)
		for (size_t goalIndex = 0; goalIndex != nGoals; goalIndex++)
			uniqueEdgeCosts.emplace(costMatrix[agentIndex][goalIndex] = distanceSquaredAgent(*agents[agentIndex], *goals[goalIndex])); // minimizar a maior distância / minimizar a maior distância ao quadrado dá na mesma. Mas o último é mais barato
	std::vector<double> sortedEdgeCosts(uniqueEdgeCosts.begin(), uniqueEdgeCosts.end());
	size_t l = 0, r = sortedEdgeCosts.size();
	Edges edgesOfBestMatching;
	while (l < r)
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

	currentTotalSumOfEachEdgeDistanceSquared = currentTotalSumOfEachEdgeDistance = 0;
	for (const auto& [agentIndex, goalIndex] : edgesOfBestMatching)
	{
		const auto& costSquared = costMatrix[agentIndex][goalIndex];
		currentTotalSumOfEachEdgeDistanceSquared += costSquared;
		currentTotalSumOfEachEdgeDistance += sqrt(costSquared);
	}

	return edgesOfBestMatching;
}

float Agent2D::getRadius()
{
	return static_cast<float>(radius);
}

vec2d& Agent2D::accessCoord()
{
	return coord;
}

Agent2D::Agent2D(const vec2d& coord, double radius, const sf::Color& color) :
	coord(coord),
	goalPtr(nullptr),
	radius(radius),
	color(color)
{
}

double distanceAgent(const Agent2D& agent, const Goal& goal)
{
	return sqrt(distanceSquaredAgent(agent, goal));
}

double distanceSquaredAgent(const Agent2D& agent, const Goal& goal)
{
	return distanceSquared(agent.coord, goal.coord);
}

void Simulator2D::SelectAction::pollEvent(const sf::Event& event)
{
	switch (event.type)
	{
	case sf::Event::KeyPressed:
	{
		switch (event.key.code)
		{
		case sf::Keyboard::Delete:
		{
			auto tryDelete = [&]
			{
				for (auto it = sim.agents.begin(); it != sim.agents.end(); it++)
				{
					if (it->get() == elemSelected)
					{
						sim.agents.erase(it);
						return true;
					}
				}
				for (auto it = sim.goals.begin(); it != sim.goals.end(); it++)
				{
					if (it->get() == elemSelected)
					{
						sim.goals.erase(it);
						return true;
					}
				}
				return false;
			};
			if (tryDelete())
			{
				switch (sim.editModeType)
				{
				case EditModeType::Free:
				{
					for (auto& agent : sim.agents)
						if (agent->goalPtr == elemSelected)
							agent->goalPtr = nullptr;
				}
				break;
				case EditModeType::Metric:
				{
					sim.updateGraphEdges();
				}
				break;
				}
				elemSelected = nullptr;
			}
		}
		break;
		}
	}
	break;
	case sf::Event::MouseButtonPressed:
	{
		switch (event.mouseButton.button)
		{
		case sf::Mouse::Left:
		{
			isHoldingLeftClick = true;
			if (elemSelected)
			{
				const auto& coord = elemSelected->accessCoord();
				auto coordDouble = sim.getCoordDouble(event.mouseButton.x, event.mouseButton.y);
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
		}
	}
	break;
	case sf::Event::MouseMoved:
	{
		auto coordDouble = sim.getCoordDouble(event.mouseMove.x, event.mouseMove.y);
		if (isHoldingLeftClick && elemSelected) // move agent
		{
			auto& coord = elemSelected->accessCoord();
			coord = coordDouble + curOff;
			if (sim.editModeType == EditModeType::Metric)
				sim.updateGraphEdges();
		}
		else
		{
			elemSelected = nullptr;
			auto curMinD2 = std::numeric_limits<double>::max();
			for (auto& agent : sim.agents)
				if (TryUpdateClosestCircle(coordDouble, curMinD2, agent->coord, agent->radius))
					elemSelected = agent.get();
			for (auto& goal : sim.goals)
				if (TryUpdateClosestCircle(coordDouble, curMinD2, goal->coord, DEFAULT_GOAL_RADIUS))
					elemSelected = goal.get();
		}
	}
	break;
	}
}

void Simulator2D::SelectAction::draw()
{
	LeftClickInterface::draw();
	if (elemSelected)
	{
		const auto& coordSelected = elemSelected->accessCoord();
		sf::CircleShape c;
		c.setFillColor(sf::Color(20, 40, 200, 140));
		c.setPosition(coordSelected);
		PrepareCircleRadius(c, elemSelected->getRadius());
		sim.window.draw(c);
	}
}

bool Simulator2D::SelectAction::canMoveView()
{
	return !elemSelected;
}

Simulator2D::SelectAction::SelectAction(Simulator2D& sim) :
	LeftClickInterface(sim),
	elemSelected(nullptr),
	isHoldingLeftClick(false)
{
}

Simulator2D::AddAgentAction::AddAgentAction(Simulator2D& sim) :
	AddItemAction(sim)
{
}

void Simulator2D::AddAgentAction::placeItem(double x, double y)
{
	sim.addAgent({ { x, y } });
}

void Simulator2D::AddGoalAction::placeItem(double x, double y)
{
	sim.addGoal({ { x, y } });
}

Simulator2D::AddGoalAction::AddGoalAction(Simulator2D& sim) :
	AddItemAction(sim)
{
}

Simulator2D::LeftClickInterface::LeftClickInterface(Simulator2D& sim) :
	sim(sim)
{
}

void Simulator2D::LeftClickInterface::draw()
{
	sim.defaultDraw();
}

bool Simulator2D::LeftClickInterface::canMoveView()
{
	return true;
}

void Simulator2D::AddItemAction::pollEvent(const sf::Event& event)
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
				auto coord = sim.window.mapPixelToCoords({ x, y });
				placeItem(static_cast<double>(coord.x), static_cast<double>(coord.y));
				if (sim.editModeType == EditModeType::Metric)
					sim.updateGraphEdges();
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
	LeftClickInterface(sim),
	didNotMoveSinceLastPress(true)
{
}

void Simulator2D::AddEdgeAction::clear()
{
	curAgent = nullptr;
	curGoal = nullptr;
}

void Simulator2D::AddEdgeAction::pollEvent(const sf::Event& event)
{
	switch (event.type)
	{
	case sf::Event::MouseMoved:
	{
		elemHoveredPtr = nullptr;
		auto curMinD2 = std::numeric_limits<double>::max();
		auto coordDouble = sim.getCoordDouble(event.mouseMove.x, event.mouseMove.y);
		for (auto& agent : sim.agents)
			if (TryUpdateClosestCircle(coordDouble, curMinD2, agent->coord, agent->radius))
				elemHoveredPtr = agent.get(), typeHovered = Type::Agent;
		for (auto& goal : sim.goals)
			if (TryUpdateClosestCircle(coordDouble, curMinD2, goal->coord, DEFAULT_GOAL_RADIUS))
				elemHoveredPtr = goal.get(), typeHovered = Type::Goal;
	}
	break;
	case sf::Event::MouseButtonPressed:
	{
		if (elemHoveredPtr)
		{
			if (typeHovered == Type::Agent)
				curAgent = static_cast<Agent2D*>(elemHoveredPtr);
			else
				curGoal = static_cast<Goal*>(elemHoveredPtr);
			elemHoveredPtr = nullptr;
		}
		else
			clear();

		if (curAgent && curGoal)
		{
			sim.addEdge(*curAgent, *curGoal);
			clear();
		}
	}
	break;
	}
}

Simulator2D::AddEdgeAction::AddEdgeAction(Simulator2D& sim) :
	LeftClickInterface(sim),
	elemHoveredPtr(nullptr),
	curAgent(nullptr),
	curGoal(nullptr)
{
}

void Simulator2D::AddEdgeAction::draw()
{
	sim.defaultDraw();

	auto overlayCircleWithColor = [&](ElemSelected* curHovered, const sf::Color& color)
	{
		const auto& coord = curHovered->accessCoord();
		const auto& r = curHovered->getRadius();
		auto& c = sim.circle;
		c.setPosition(coord);
		c.setFillColor(color);
		PrepareCircleRadius(c, r);
		sim.window.draw(c);
	};

	if (elemHoveredPtr && elemHoveredPtr != curAgent && elemHoveredPtr != curGoal)
	{
		overlayCircleWithColor(elemHoveredPtr, sf::Color::Cyan);
	}

	static const sf::Color SELECTING_VERTEX_FOR_NEW_ADD_COLOR = sf::Color(0xff9933ff);

	if (curAgent) overlayCircleWithColor(curAgent, SELECTING_VERTEX_FOR_NEW_ADD_COLOR);
	if (curGoal)
		overlayCircleWithColor(curGoal, SELECTING_VERTEX_FOR_NEW_ADD_COLOR);
}
