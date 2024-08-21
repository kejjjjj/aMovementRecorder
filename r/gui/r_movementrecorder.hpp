#pragma once

#include "r/gui/r_gui.hpp"

struct ImKeybind;

class CMovementRecorderWindow : public CGuiElement
{
public:
	CMovementRecorderWindow(const std::string& id);
	~CMovementRecorderWindow();

	void* GetRender() override {
		union {
			void (CMovementRecorderWindow::* memberFunction)();
			void* functionPointer;
		} converter{};
		converter.memberFunction = &CMovementRecorderWindow::Render;
		return converter.functionPointer;
	}

	void Render() override;

private:
	std::vector<std::unique_ptr<ImKeybind>> m_oKeybinds;

	size_t m_uSelectedPlayback = {};
};
