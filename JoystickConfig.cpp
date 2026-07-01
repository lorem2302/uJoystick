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

#include "MBUtils.h"

using namespace std;

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

bool Joystick::_loadButtonSettings(const std::string& settings) {
    
    bool handled = false;

    if (_assignedButtonActions.size() != (m_numButtons + 4*m_numHats)) { //solo quando cambia m_numButtons
        _assignedButtonActions.clear();
        _assignedButtonActions.resize(m_numButtons + 4*m_numHats);
    }

    stringstream ss(settings);
    string item;
    int id = -1;
    string actionName;
    string actionName2;
    bool* action2 = nullptr;
    bool repeat = false;

    while (getline(ss, item, ',')) { //un ciclo per ogni elemento di 'BUTTON'
            stringstream itemStream(item);

            std::string name, values;
            getline(itemStream, name, ':'); //salva la parte prima dell'uguale in 'name'
            getline(itemStream, values);
            //transform(name.begin(), name.end(), name.begin(), ::toupper);
            name = stripBlankEnds(toupper(name));
            //name = stripBlankEnds(name);
            
            values = stripBlankEnds(values);
            int ivals = atoi(values.c_str());
            bool bvals = (strcasecmp(values.c_str(), "TRUE") == 0 || ivals != 0);
            if (name == "ID") {
                id = ivals;
                handled = true;
            }
            else if (id >= 0){
                                
                if (name == "ACTION") {
                    actionName = values;
                    handled &= true;
                }
                else if (name == "ACTION2") {
                    actionName2 = values;
                    handled &= true;
                }
                else if (name == "2ND_FCN") {
                    action2 = m_stateMap.at(toupper(values)); // Bottone che premuto dà accesso alla seconda funzione del tasto //PUNTATORE ALLA VARIABILE CHE DECIDE LA SECONDA FUNZIONE DEI TASTI
                    handled &= true;
                }
                else if (name == "REPEAT") {
                    repeat = bvals;
                    handled &= true;
                }
                else
                    handled &= false;
            }
            else {
                std::cerr << id << " is not a valid button ID." << std::endl;
                handled = false;
                }
        }

    int time = SDL_GetPerformanceCounter();

    AssignedButtonAction action;
    action.actionName = actionName;
    action.actionName2 = actionName2;
    action.action2 = action2;
    action.assigned = true;
    action.repeat = repeat;
    action.time = time;
    _assignedButtonActions[id] = action;

    return handled;
    
}
