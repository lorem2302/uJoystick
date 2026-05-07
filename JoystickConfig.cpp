/*****************************************************************/
/*    NAME:				                                         */
/*    ORGN: Mission Systems Pty Ltd                              */
/*    (based on QGroundControl Joystick.cc)  				     */
/*    FILE: JoystickConfig.cpp                                   */
/*    DATE: 30 Apr 2026                                          */
/*****************************************************************/
#include <stdio.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include "Joystick.h"
#include "JoystickConfig.h"

namespace
{
    static constexpr const char* kButtonActionArrayGroup = "JoystickButtonActionSettingsArray";
    static constexpr const char* kButtonActionNameKey = "actionName";
    static constexpr const char* kButtonRepeatKey = "repeat";
    static constexpr const char* kAxisSettingsArrayGroup = "JoystickAxisSettingsArray";
    static constexpr const char* kAxisFunctionKey = "function";
    static constexpr const char* kAxisMinKey = "min";
    static constexpr const char* kAxisMaxKey = "max";
    static constexpr const char* kAxisCenterKey = "center";
    static constexpr const char* kAxisDeadbandKey = "deadband";
    static constexpr const char* kAxisReversedKey = "reversed";
}

bool Joystick::_validAxis(int axis) const
{
    if ((axis >= 0) && (axis < m_numAxes)) {
        return true;
    }

    std::cerr << "Invalid axis index" << axis << std::endl;
    return false;
}

/*
void Joystick::_setJoystickAxisForAxisFunction(AxisFunction_t axisFunction, int joystickAxis)
{
    if (axisFunction == maxAxisFunction) {
        std::cerr << "Internal Error: maxAxisFunction passed to _setJoystickAxisForAxisFunction" << std::endl;
        return;
    }
    if (joystickAxis == AXIS_NOT_SET) {
        std::cerr << "Internal Error: AXIS_NOT_SET passed to _setJoystickAxisForAxisFunction" << std::endl;
        return;
    }

    if (!_validAxis(joystickAxis)) { //se true (_validAxis restituisce false), allora esci dalla funzione
        return;
    }

    _axisFunctionToJoystickAxisMap[axisFunction] = joystickAxis; //fissa axisFunction associandola nella mappa al corrispondente asse
}

void Joystick::_resetFunctionToAxisMap()
{
    _axisFunctionToJoystickAxisMap.clear();
    for (int i = 0; i < maxAxisFunction; i++) { //enum: insieme di interi numerati, maxAxisFunction viene interpretato come se fosse un intero
        _axisFunctionToJoystickAxisMap[static_cast<AxisFunction_t>(i)] = AXIS_NOT_SET; //ogni iterazione del ciclo for scorre un nuovo elemento di _axisFunctionToJoystickAxisMap e associa AXIS_NOT_SET
    }
}

void Joystick::_resetAxisCalibrationData() //chiamare questa per chiamare anche _resetFunctionToAxisMap
{
    _resetFunctionToAxisMap();
    for (int axis = 0; axis < m_numAxes; axis++) {
        auto& calibration = _rgCalibration[axis]; //'auto&' deduci automaticamente il tipo e agisci sull'sitanza, non sulla copia
        calibration.reset();
    }
}

void Joystick::_resetButtonActionData()
{
    for (int i = 0; i < _assignedButtonActions.count(); i++) {
        delete _assignedButtonActions[i];
        _assignedButtonActions[i] = nullptr;
    }
}
*/

void Joystick::_resetButtonEventStates()
{
    for (size_t i = 0; i < _buttonEventStates.size(); i++) {
        _buttonEventStates[i] = ButtonEventNone;
    }
}

void Joystick::_loadButtonSettings(const std::string& path) {
    std::ifstream file(path); //apertura del file per la lettura
    nlohmann::json j;
    file >> j;

    _assignedButtonActions.clear();
    _assignedButtonActions.resize(m_numButtons + (4 * m_numHats));

    for (const auto& btn : j["buttons"]) { //scorre tutti gli elementi "buttons" del file json
        int id = btn["id"];
        std::string actionName = btn["action"];
        bool repeat = btn.value("repeat", false); //false nel caso in cui non esista il campo 'repeat'

        if (id < 0 || id >= (m_numButtons + (4 * m_numHats)))
            continue;

        int time = SDL_GetPerformanceCounter();

        AssignedButtonAction action;
        action.actionName = actionName;
        action.assigned = true;
        action.repeat = repeat;
        action.time = time;
        _assignedButtonActions[id] = action;
    }
}
