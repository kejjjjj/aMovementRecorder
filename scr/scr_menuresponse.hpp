#pragma once

void Script_OpenScriptMenu();
void Script_ScriptMenuResponse();
void Script_ParseMenuResponse(char* text);
void Script_OnMenuResponse(int serverId, int menu, const char* response);

void Script_OnPositionLoaded();
void Script_OnPositionSaved();
