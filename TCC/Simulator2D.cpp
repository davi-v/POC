#include "Pch.hpp"
#include "Simulator2D.hpp"

#include "Hungarian.h"
#include "Hopcroft.hpp"

void Simulator2D::addMessage(const char* msg)
{
	warnings.emplace_back(msg, clock.getElapsedTime());
}

bool Simulator2D::loadAgentsAndGoals(const std::string& path)
{
	if (auto f = std::ifstream(path, std::ios::binary | std::ios::ate))
	{
		auto retError = [&]
		{
			LOG_ERROR("Error reading file ", path, '\n');
			return false;
		};

		size_t fileSize = f.tellg();
		auto nextSize = sizeof(size_t);
		if (fileSize < nextSize)
			return retError();

		f.seekg(0);

		size_t nAgents;
		FileRead(f, nAgents);

		fileSize -= nextSize;

		nextSize = nAgents * sizeof(Coord);
		if (fileSize < nextSize)
			return retError();
		agents.clear();
		agents.reserve(nAgents);
		for (size_t i = 0; i != nAgents; i++)
		{
			Coord coord;
			FileRead(f, coord);
			agents.emplace_back(coord);
		}

		fileSize -= nextSize;
		auto div = nextSize / sizeof(Coord);
		auto mod = nextSize % sizeof(Coord);
		if (mod != 0)
			return retError();
		if (nAgents == 0 && div == 0)
			return retError(); // não aceite arquivo sem nenhum vértice

		goals.resize(div);
		for (size_t i = 0; i != div; i++)
			FileRead(f, goals[i]);


		return true;
	}
	return false;
}

void Simulator2D::saveAgentsAndGoals(const std::string& path)
{
	if (auto f = std::ofstream(path, std::ios::binary))
	{
		auto nAgents = agents.size();
		FileAppend(f, nAgents);
		for (const auto& agent : agents) FileAppend(f, agent.coord);
		for (const auto& goal : goals) FileAppend(f, goal);
	}
}

void Simulator2D::updateZoomFacAndViewSizeFromZoomLevel(sf::View& view)
{
	zoomFac = powf(2, static_cast<float>(zoomLevel) / N_SCROLS_TO_DOUBLE);
	view.setSize(sf::Vector2f{ window.getSize() } * zoomFac);
}

Simulator2D::Simulator2D(sf::RenderWindow& window) :
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

	textMetric.setFont(font);

	textPopUpMessages.setFont(font);
	textPopUpMessages.setFillColor(sf::Color::Red);
}

void Simulator2D::addAgent(const Agent2D& agent)
{
	agents.emplace_back(agent);
	tryUpdateGraph();
}

void Simulator2D::addGoal(const Coord& coord)
{
	goals.emplace_back(coord);
	tryUpdateGraph();
}

void Simulator2D::tick()
{
}

void Simulator2D::tickAndDraw()
{
	if (sim)
	{
		std::cout << sim->getGlobalTime() << '\n';

		updatePreferredVelocities();
		sim->doStep();
	}

	for(size_t i = 0, end = agents.size(); i != end; i++)
	{
		const auto& agent = agents[i];

		circle.setFillColor(agent.color);
		auto radiusRendered = static_cast<float>(agent.radius);
		circle.setRadius(radiusRendered);
		auto& origin = radiusRendered;
		circle.setOrigin(origin, origin);

		sf::Vector2f coordUsing;

		if (sim)
		{
			auto& coord = sim->getAgentPosition(i);
			coordUsing = { coord.x(), coord.y() };
		}
		else
			coordUsing = agent.coord;

		circle.setPosition(coordUsing);

		window.draw(circle);
		if (agent.goalPtr)
		{
			sf::Color color;
			if (distanceSquared(agent, *agent.goalPtr) == currentMaxEdgeSquared)
				color = sf::Color::Yellow;
			else
				color = sf::Color::White;
			sf::Vertex vertices[2]
			{
				sf::Vertex{ coordUsing, color },
				sf::Vertex{ *agent.goalPtr, color },
			};
			window.draw(vertices, 2, sf::Lines);
		}
	}

	circle.setFillColor(GOAL_COLOR);
	circle.setRadius(DEFAULT_RADIUS);
	circle.setOrigin(DEFAULT_RADIUS, DEFAULT_RADIUS);
	for (const auto& goal : goals)
	{
		circle.setPosition(goal);
		window.draw(circle);
	}

	// draw UI in screen space
	sf::View curView = window.getView(); // to restore later
	sf::View orgView = window.getDefaultView();
	window.setView(orgView);

	static constexpr const char* METRIC_MESSAGES[]
	{
		"minimize total sum of edges",
		"minimize biggest edge",
		"minimize biggest edge then minimize total sum of edges"
	};
	std::string metricStr;
	metricStr += "Agents: " + std::to_string(agents.size());
	metricStr += "\nGoals: " + std::to_string(goals.size());
	metricStr += '\n';

	metricStr += "Metric: ";
	metricStr += METRIC_MESSAGES[static_cast<int>(metric)];
	// na métrica de apenas minimizar a maior aresta, não faz diferença se a distância considerada foi quadrática ou não
	if (metric != Metric::MinimizeBiggestEdge)
		if (distanceFuncUsingCallback == distanceSquared)
			metricStr += " squared";

	if (condCanCompute())
	{
		metricStr += "\nMax edge: " + std::to_string(currentMaxEdge);
		metricStr += "\nSum of edges: " + std::to_string(currentTotalSumOfEachEdgeDistance);
		metricStr += "\nSum of each edge squared: " + std::to_string(currentTotalSumOfEachEdgeDistanceSquared);
	}


	textMetric.setString(metricStr);
	window.draw(textMetric);
	
	// pop-up messages
	std::string str;
	for (auto it = warnings.begin(); it != warnings.end(); )
	{
		auto& msg = it->first;
		auto& ts = it->second;
		auto now = clock.getElapsedTime();
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
	case sf::Event::MouseButtonPressed:
	{
		auto& data = event.mouseButton;
		switch (data.button)
		{
		case sf::Mouse::Left:
		{
			lastLeftX = lastPxClickedX = data.x;
			lastLeftY = lastPxClickedY =  data.y;
		}
		break;
		case sf::Mouse::Right:
		{
			lastRightX = data.x;
			lastRightY = data.y;
		}
		break;
		}
	}
	break;
	case sf::Event::MouseMoved:
	{
		if (lastPxClickedX != -1)
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

			if (!sim)
			{
				auto& x = event.mouseButton.x;
				auto& y = event.mouseButton.y;
				if (x == lastLeftX && y == lastLeftY)
				{
					auto coord = window.mapPixelToCoords({ x, y });
					addAgent({ { static_cast<double>(coord.x), static_cast<double>(coord.y) } });
				}
			}
		}
		break;
		case sf::Mouse::Right:
		{
			if (!sim)
			{
				auto& x = event.mouseButton.x;
				auto& y = event.mouseButton.y;
				if (x == lastRightX && y == lastRightY)
				{
					auto coord = window.mapPixelToCoords({ x, y });
					addGoal({ static_cast<double>(coord.x), static_cast<double>(coord.y) });
				}
			}
		}
		break;
		}
	}
	break;
	case sf::Event::KeyPressed:
	{
		switch (event.key.code)
		{
		case sf::Keyboard::C:
		{
			if (sim)
				sim.reset();
			Clear(agents, goals);
		}
		break;
		case sf::Keyboard::R:
		{
			restartRVO2();
		}
		break;
		case sf::Keyboard::Enter:
		{
			if (sim)
				sim.reset();
			else
				restartRVO2();
		}
		break;
		case sf::Keyboard::T:
		{
			if (circle.getOutlineThickness() == 0)
				circle.setOutlineThickness(-1);
			else
				circle.setOutlineThickness(0);
		}
		break;
		case sf::Keyboard::S:
		{
			if (!agents.empty() || !goals.empty())
			{
				auto selection = tinyfd_saveFileDialog( // there is also a wchar_t version
					"Select where to save the current agents and goals", // title
					nullptr, // optional initial directory
					1, // number of filter patterns
					lFilterPatterns, // char const * lFilterPatterns[2] = { "*.txt", "*.jpg" };
					nullptr
				);
				if (selection)
					saveAgentsAndGoals(selection);
			}
		}
		break;
		case sf::Keyboard::O:
		{
			sim.reset();
			auto selection = tinyfd_openFileDialog( // there is also a wchar_t version
				"Select map to load", // title
				nullptr, // optional initial directory
				1, // number of filter patterns
				lFilterPatterns, // char const * lFilterPatterns[2] = { "*.txt", "*.jpg" };
				NULL, // optional filter description
				0 // forbid multiple selections
			);
			if (selection)
				if (loadAgentsAndGoals(selection))
				{
					tryUpdateGraph();

					// vamos colocar o centro da tela na média dos pontos
					double sX = 0, sY = 0;
					for (const auto& agent : agents)
					{
						sX += agent.coord.x;
						sY += agent.coord.y;
					}
					for (const auto& goals : goals)
					{
						sX += goals.x;
						sY += goals.y;
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
		break;
		case sf::Keyboard::M:
		{
			metric = static_cast<Metric>((static_cast<int>(metric) + 1) % static_cast<int>(Metric::Count));
			tryUpdateGraph();
		}
		break;
		case sf::Keyboard::W:
		{
			distanceFunctionUsingType = static_cast<DistanceFunctionsEnum>((static_cast<DistanceFunctionsUnderlyingType>(distanceFunctionUsingType) + 1) % static_cast<DistanceFunctionsUnderlyingType>(DistanceFunctionsEnum::Count));
			distanceFuncUsingCallback = DISTANCE_FUNCTIONS[static_cast<DistanceFunctionsUnderlyingType>(distanceFunctionUsingType)];
			tryUpdateGraph();
		}
		break;
		}
	}
	break;
	}
}

void Simulator2D::restartRVO2()
{
	MakeUniquePtr(sim,
		TIME_STEP, // timestep
		150.f, // neighbour dist
		100, // max neighbours
		100.f, // time horizon
		100.f, // time horizon obst
		DEFAULT_RADIUS * 1.2f, // radius
		DEFAULT_VELOCITY // maxSpeed
	);
	for (const auto& agent : agents)
		sim->addAgent(RVO::Vector2(static_cast<float>(agent.coord.x), static_cast<float>(agent.coord.y)));
}

void Simulator2D::updatePreferredVelocities()
{
#ifdef _OPENMP
#pragma omp parallel for
#endif
	for (size_t i = 0, end = agents.size(); i != end; i++)
	{
		const auto& agent = agents[i];

		RVO::Vector2 goal;
		if (agent.goalPtr)
			goal = *agent.goalPtr;
		else
			goal = agent.coord;

		auto goalVector = goal - sim->getAgentPosition(i);
		if (goalVector != RVO::Vector2(0, 0))
			goalVector = RVO::normalize(goalVector) * DEFAULT_VELOCITY;
		sim->setAgentPrefVelocity(i, goalVector);
	}
}

bool Simulator2D::reachedGoal()
{
	/* Check if all agents have reached their goals. */
	for (size_t i = 0, end = agents.size(); i != end; i++)
		if (RVO::absSq(sim->getAgentPosition(i) - goals[i]) > sim->getAgentRadius(i) * sim->getAgentRadius(i))
			return false;
	return true;
}

bool Simulator2D::condCanCompute()
{
	auto
		nAgents = agents.size(),
		nGoals = goals.size();
	return nAgents && nAgents >= nGoals;
}

void Simulator2D::tryUpdateGraph()
{
	auto
		nAgents = agents.size(),
		nGoals = goals.size();
	if (condCanCompute())
	{
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
				agent.goalPtr = nullptr;
			for (auto& [agentIndex, goalIndex] : getEdgesMinimizingBiggestEdgeAndUpdateInternalVariables())
				agents[agentIndex].updateGoal(goals[goalIndex]);
		}
		break;
		case Metric::Both:
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
	else
		for (auto& agent : agents)
			agent.goalPtr = nullptr;
}

void Simulator2D::updateAgentsGoalsAndInternalVariablesWithHungarianAlgorithm(CostMatrix& costMatrix, bool needsToUpdateMaxEdge)
{
	HungarianAlgorithm HungAlgo;
	std::vector<int> assignment;

	auto
		nAgents = agents.size(),
		nGoals = goals.size();

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

	for (size_t agentIndex = 0; agentIndex != nAgents; agentIndex++)
	{
		auto& agent = agents[agentIndex];
		auto& goalIndex = assignment[agentIndex];
		if (goalIndex < nGoals) // agente foi escolhido para ir a algum goal
		{
			agent.updateGoal(goals[goalIndex]);
			auto edgeCost = costMatrix[agentIndex][goalIndex];
			double edgeCostSquared;
			if (distanceFunctionUsingType == DistanceFunctionsEnum::SumOfEachEdgeDistance)
			{
				edgeCostSquared = distanceSquared(agents[agentIndex], goals[goalIndex]); // temos que calcular do mesmo jeito em todos os lugares para evitar erros de ponto flutuante
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
			agent.goalPtr = nullptr;
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
			costMatrix[agentIndex][goalIndex] = distanceFuncUsingCallback(agent, goals[goalIndex]);
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
			uniqueEdgeCosts.emplace(costMatrix[agentIndex][goalIndex] = distanceSquared(agents[agentIndex], goals[goalIndex])); // minimizar a maior distância / minimizar a maior distância ao quadrado dá na mesma. Mas o último é mais barato
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

Agent2D::Agent2D(const Coord& coord, double radius, const sf::Color& color) :
	coord(coord),
	goalPtr(nullptr),
	radius(radius),
	color(color)
{
}

void Agent2D::updateGoal(Goal& goal)
{
	this->goalPtr = &goal;
}

Coord::operator sf::Vector2f() const
{
	return { static_cast<float>(x), static_cast<float>(y) };
}

Coord::operator RVO::Vector2() const
{
	return RVO::Vector2(static_cast<float>(x), static_cast<float>(y));
}

double distance(const Agent2D& agent, const Goal& goal)
{
	return sqrt(distanceSquared(agent, goal));
}

double distanceSquared(const Agent2D& agent, const Goal& goal)
{
	auto dx = agent.coord.x - goal.x;
	auto dy = agent.coord.y - goal.y;
	return dx * dx + dy * dy;
}
