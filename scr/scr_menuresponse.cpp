#include "scr_menuresponse.hpp"
#include <cg/cg_local.hpp>
#include <cg/cg_offsets.hpp>
#include <unordered_map>
#include <functional>
#include <movement_recorder/mr_main.hpp>
#include <string>
#include <iostream>

using namespace std::string_literals;

void Script_ParseMenuResponse(char* text) //this will always be the same format so hardcoding everything should not cause issues
{

    std::string unfixed = text;
    int sv_serverId;
    int menu;
    const char* response;

    //skip first 7 chars
    unfixed = unfixed.substr(7);

    size_t pos = unfixed.find_first_of(' ');
    std::string str = unfixed.substr(0, pos);

    sv_serverId = std::stoi(str);
    unfixed = unfixed.substr(pos + 1);

    pos = unfixed.find_first_of(' ');
    str = unfixed.substr(0, pos);
    menu = std::stoi(str);

    unfixed = unfixed.substr(pos + 1);
    pos = unfixed.find_last_of('\n');
    str = unfixed.substr(0, pos);
    response = str.c_str();

    Script_OnMenuResponse(sv_serverId, menu, response);


}
__declspec(naked) void Script_ScriptMenuResponse()
{
    const static std::uint32_t fnc = 0x4F8D90;
    const static std::uint32_t retn = 0x54DE5E;
    char* text;
    // itemDef_s* _item;
    __asm
    {
        mov text, eax; //eax holds the va
        //mov edx, _item; //edx holds the itemdef ptr
        //mov edx, [esp + 8];
        //mov _item, edx;
        call fnc;
        pop edi;
        pop esi;
        pop ebx;
        add esp, 0x400;
        // push _item;
        push text;
        call Script_ParseMenuResponse; //call Script_ParseMenuResponse to steal the response
        add esp, 0x4;
        retn;
    }
}
__declspec(naked) void Script_OpenScriptMenu()
{
    const static std::uint32_t fnc = 0x4F8D90;
    const static std::uint32_t retn = 0x46D4D8;
    static char* text{};
    __asm
    {
        mov text, eax;
        add esp, 0x10;
        pop esi;
        pop ebx;
        pop edi;
        xor ecx, ecx;
        pop ebp;
        call fnc;
        push text;
        call Script_ParseMenuResponse;
        add esp, 0x4;
        retn;
    }
}
void Script_OnMenuResponse([[maybe_unused]] int serverId, int menu, [[maybe_unused]] const char* response)
{

    const auto getMenuName = [](int _menu) -> char* {
        int* stringoffs = &clients->gameState.stringOffsets[1970];
        return &clients->gameState.stringData[*(stringoffs + _menu)];
    };

    char* menu_name = getMenuName(menu);

    if (!menu_name)
        return;

    using response_func_t = std::unordered_map<std::string, std::function<void()>>;
    using menu_response_t = std::unordered_map<std::string, response_func_t>;

    response_func_t response_fnc = {
        {"save", Script_OnPositionSaved},
        {"load", Script_OnPositionLoaded}
    };

    menu_response_t menu_names = {
        { "cj", response_fnc}
    };

    try {

        if (!menu_name || strlen(menu_name) <= 1) {
            //berrytrials doesn't include the menuname
            return response_fnc.at(response)();      
        }

        auto& menu_t = menu_names.at(menu_name);
        auto& func = menu_t.at(response);

        func();
    }
    catch ([[maybe_unused]] std::out_of_range& ex) {
        return;
    }
}

void Script_OnPositionLoaded()
{
    CStaticMovementRecorder::Instance->OnPositionLoaded();
}
void Script_OnPositionSaved()
{
}