#pragma once

#include "r/gui/r_gui.hpp"

class CMovementRecorderWindow : public CGuiElement
{
public:
	CMovementRecorderWindow(const std::string& id);
	~CMovementRecorderWindow() = default;

	void* GetRender() override {
		union {
			void (CMovementRecorderWindow::* memberFunction)();
			void* functionPointer;
		} converter{};
		converter.memberFunction = &CMovementRecorderWindow::Render;
		return converter.functionPointer;
	}

	void Render() override;
	//const std::string& GetIdentifier() const override { return id; }

private:


	size_t m_uSelectedPlayback = {};
};
