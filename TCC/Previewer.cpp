#include "Pch.hpp"
#include "Previewer.hpp"

Previewer::Previewer(ViewerBase& viewerBase) :
	viewerBase(viewerBase)
{
}

bool Previewer::drawUI()
{
	bool opened = true;
	ImGui::Begin((std::string{ "Heuristic - " } + getTitle()).c_str(), & opened);
	drawUIImpl();
	ImGui::End();
	return opened;
}