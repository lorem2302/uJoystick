/*****************************************************************/
/*    NAME:				                                         */
/*    ORGN: Mission Systems Pty Ltd                              */
/*    (based on QGroundControl Joystick.cc)  				     */
/*    FILE: JoystickUtils.cpp                                    */
/*    DATE: 30 Apr 2026                                          */
/*****************************************************************/
#include <stdio.h>
#include <functional>
#include "Joystick.h"
#include "JoystickUtils.h"

namespace {
static constexpr const char* _buttonActionNone = "_buttonActionNone";
static constexpr const char* _buttonActionArm = "_buttonActionArm";
static constexpr const char* _buttonActionDisarm = "_buttonActionDisarm";
static constexpr const char* _buttonActionVTOLFixedWing = "_buttonActionVTOLFixedWing";
static constexpr const char* _buttonActionVTOLMultiRotor = "_buttonActionVTOLMultiRotor";
static constexpr const char* _buttonActionTriggerCamera = "_buttonActionTriggerCamera";
static constexpr const char* _buttonActionContinuousZoomIn = "_buttonActionContinuousZoomIn";
static constexpr const char* _buttonActionContinuousZoomOut = "_buttonActionContinuousZoomOut";
static constexpr const char* _buttonActionStepZoomIn = "_buttonActionStepZoomIn";
static constexpr const char* _buttonActionStepZoomOut = "_buttonActionStepZoomOut";
static constexpr const char* _buttonActionNextStream = "_buttonActionNextStream";
static constexpr const char* _buttonActionPreviousStream = "_buttonActionPreviousStream";
static constexpr const char* _buttonActionNextCamera = "_buttonActionNextCamera";
static constexpr const char* _buttonActionPreviousCamera = "_buttonActionPreviousCamera";
static constexpr const char* _buttonActionGimbalUp = "_buttonActionGimbalUp";
static constexpr const char* _buttonActionGimbalDown = "_buttonActionGimbalDown";
static constexpr const char* _buttonActionGimbalLeft = "_buttonActionGimbalLeft";
static constexpr const char* _buttonActionGimbalRight = "_buttonActionGimbalRight";
static constexpr const char* _buttonActionStartVideoRecord = "_buttonActionStartVideoRecord";
static constexpr const char* _buttonActionStopVideoRecord = "_buttonActionStopVideoRecord";
static constexpr const char* _buttonActionToggleVideoRecord = "_buttonActionToggleVideoRecord";
static constexpr const char* _buttonActionGimbalCenter = "_buttonActionGimbalCenter";
static constexpr const char* _buttonActionGimbalYawLock = "_buttonActionGimbalYawLock";
static constexpr const char* _buttonActionGimbalYawFollow = "_buttonActionGimbalYawFollow";
static constexpr const char* _buttonActionEmergencyStop = "_buttonActionEmergencyStop";
static constexpr const char* _buttonActionMotorInterlockEnable = "_buttonActionMotorInterlockEnable";
static constexpr const char* _buttonActionMotorInterlockDisable = "_buttonActionMotorInterlockDisable";
}

/*
int _getAxisValue(int idx);
int _getJoystickAxisForAxisFunction(AxisFunction_t axisFunction);
float _adjustRange(int value, const AxisCalibration_t& calibration, bool withDeadbands);

bool _getButton(int idx);
void _updateButtonEventState(const bool buttonPressed, ButtonEvent_t& buttonEventState);
bool _getHat(int hat, int idx);
void _updateButtonEventStates(std::vector<ButtonEvent_t>& buttonEventStates);
void _executeButtonAction(const std::string& action, const ButtonEvent_t buttonEvent)
*/

//------------------------------------------------------------
// Procedure: _handleAxis()

void Joystick::_handleAxis()
{
    const int axisDelay = static_cast<int>(1000.0 / m_axisFreq); //tempo tra due aggiornamenti in ms
    static Uint64 last = 0;
    std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
    Uint64 now = SDL_GetPerformanceCounter();
    Uint64 freq = SDL_GetPerformanceFrequency();
    if (((now - last)/freq) <= axisDelay) {
        last = now;
        return;
    }
    std::cout << "????????????????????????????????????????????" << std::endl;
    //_axisElapsedTimer.start();

    if (m_pollingFor == NotPolling) {
        std::cerr << "Internal Error: Joystick not polling!" << std::endl;
        
        now = SDL_GetPerformanceCounter();
        last = now;
        
        return;
    }

    else if (m_pollingFor == PollingForConfiguration) {
        // Signal the axis values to Joystick Config/Cal. Axes in Joystick terminology are the same as Channels in
        // RemoteControlCalibrationController terminology.
        std::vector<int> channelValues(m_numAxes);
             
        for (int axisIndex = 0; axisIndex < m_numAxes; axisIndex++) {
            channelValues[axisIndex] = _getAxisValue(axisIndex);
            std::cout << "Axis " << axisIndex << " - value " << channelValues[axisIndex] << std::endl;
            std::string str = "AXIS_" + std::to_string(axisIndex);

            now = SDL_GetPerformanceCounter();
            last = now;

            Notify(str, channelValues[axisIndex]);
        }
    }

    else if (m_pollingFor == PollingForVehicle) {
        bool useDeadband = m_deadband;
        bool throttleModeCenterZero = m_throttleModeCenterZero;
        bool negativeThrust = m_negativeThrust;
        bool circleCorrection = m_circleCorrection;
        bool throttleSmoothing = m_throttleSmoothing;
        double exponentialPercent = m_exponentialPercent;

        if (_getJoystickAxisForAxisFunction(rollFunction) == -1 || _getJoystickAxisForAxisFunction(pitchFunction) == -1 ||
            _getJoystickAxisForAxisFunction(yawFunction) == -1 || _getJoystickAxisForAxisFunction(throttleFunction) == -1) {
            std::cerr << "Internal Error: Missing attitude control axis function mapping!" << std::endl;

            now = SDL_GetPerformanceCounter();
            last = now;

            return;
        }
        int axisIndex = _getJoystickAxisForAxisFunction(rollFunction);
        float roll = _adjustRange(_getAxisValue(axisIndex), m_axisCalibration[axisIndex], useDeadband);
        axisIndex = _getJoystickAxisForAxisFunction(pitchFunction);
        float pitch = _adjustRange(_getAxisValue(axisIndex), m_axisCalibration[axisIndex], useDeadband);
        axisIndex = _getJoystickAxisForAxisFunction(yawFunction);
        float yaw = _adjustRange(_getAxisValue(axisIndex), m_axisCalibration[axisIndex], useDeadband);
        axisIndex = _getJoystickAxisForAxisFunction(throttleFunction);
        float throttle = _adjustRange(_getAxisValue(axisIndex), m_axisCalibration[axisIndex], throttleModeCenterZero ? useDeadband : false); //se throttleModeCenterZero è vero allora è possibile usare Deadband

        if (throttleSmoothing) {
            static float throttleSmoothingValue = 0.f;
            throttleSmoothingValue += (throttle * (40 / 1000.f)); // for throttle to change from min to max it will take 1000ms (40ms is a loop time)
            throttleSmoothingValue = std::max(static_cast<float>(-1.f), std::min(throttleSmoothingValue, static_cast<float>(1.f)));
            throttle = throttleSmoothingValue;
        }

        if (circleCorrection) {
            const float roll_limited = std::max(static_cast<float>(-M_PI_4), std::min(roll, static_cast<float>(M_PI_4)));
            const float pitch_limited = std::max(static_cast<float>(-M_PI_4), std::min(pitch, static_cast<float>(M_PI_4)));
            const float yaw_limited = std::max(static_cast<float>(-M_PI_4), std::min(yaw, static_cast<float>(M_PI_4)));
            const float throttle_limited = std::max(static_cast<float>(-M_PI_4), std::min(throttle, static_cast<float>(M_PI_4)));

            // Map from unit circle to linear range and limit
            roll = std::max(-1.0f, std::min(tanf(asinf(roll_limited)), 1.0f));
            pitch = std::max(-1.0f, std::min(tanf(asinf(pitch_limited)), 1.0f));
            yaw = std::max(-1.0f, std::min(tanf(asinf(yaw_limited)), 1.0f));
            throttle = std::max(-1.0f, std::min(tanf(asinf(throttle_limited)), 1.0f));
        }

        if (exponentialPercent > 0.0) {
            const float exponential = -static_cast<float>(exponentialPercent / 100.0);
            roll = -exponential * powf(roll, 3) + ((1 + exponential) * roll);
            pitch = -exponential * powf(pitch, 3) + ((1 + exponential) * pitch);
            yaw = -exponential * powf(yaw, 3) + ((1 + exponential) * yaw);
        }

        // Adjust throttle to 0:1 range
        if (throttleModeCenterZero) {
            if (!negativeThrust) {
                throttle = std::max(0.0f, throttle);
            }
        }
        else {
            throttle = (throttle + 1.0f) / 2.0f;
        }

        // NOTE: The buttonPressedBits going to MANUAL_CONTROL are currently used by ArduSub (and it only handles 16 bits)
        // Set up button bitmap
        uint64_t buttonPressedBits = 0;  ///< Buttons pressed for manualControl signal
        for (int buttonIndex = 0; buttonIndex < m_numButtons; buttonIndex++) {
            const uint64_t buttonBit = static_cast<uint64_t>(1LL << buttonIndex); //bit a sx di 'buttonIndex' posizioni
            if (_buttonEventStates[buttonIndex] == ButtonEventDownTransition || _buttonEventStates[buttonIndex] == ButtonEventRepeat) {
                buttonPressedBits |= buttonBit; //OR bit a bit
            }
        }

        now = SDL_GetPerformanceCounter();
        last = now;

        Notify("ROLL", roll);
        Notify("PITCH", pitch);
        Notify("YAW", yaw);
        Notify("THROTTLE", throttle);

        //const uint16_t lowButtons = static_cast<uint16_t>(buttonPressedBits & 0xFFFF); //a partire dai LSB
        //const uint16_t highButtons = static_cast<uint16_t>((buttonPressedBits >> 16) & 0xFFFF); //prende i bit 16-31 e fa operazione di AND bit a bit con 1111 1111 .... 1111

        //MAVLINK
        //vehicle->sendJoystickDataThreadSafe(roll, pitch, yaw, throttle, lowButtons, highButtons, pitchExtension, rollExtension, aux1, aux2, aux3, aux4, aux5, aux6);
    }

}

//------------------------------------------------------------
// helper functions for _handleAxis()

int Joystick::_getAxisValue(int idx) const
{
    if (idx < 0) {
        return 0;
    }

    if (m_gameController) {
        return SDL_GameControllerGetAxis(m_gameController, static_cast<SDL_GameControllerAxis>(idx)); //SDL_GameControllerAxis è un enum che definisce gli assi disponibili, _gamepadAxes[idx]
    }
    else {
        return SDL_JoystickGetAxis(m_joystick, idx);
    }
}

int Joystick::_getJoystickAxisForAxisFunction(AxisFunction_t axisFunction) const //non passo axisMap, è membro della classe e dicharata in Joystick.h
{
    auto it = m_axisMap.find(axisFunction);
    return (it != m_axisMap.end()) ? it->second : -1; //ritorna il secondo elemento del dizionario, oppure -1
}

float Joystick::_adjustRange(int value, const AxisCalibration_t& calibration, bool withDeadbands) const
{
    float valueNormalized;
    float axisLength;
    float axisBasis;

    if (value > calibration.center) {
        axisBasis = 1.0f;
        valueNormalized = value - calibration.center;
        axisLength = calibration.max - calibration.center;
    }
    else {
        axisBasis = -1.0f;
        valueNormalized = calibration.center - value;
        axisLength = calibration.center - calibration.min;
    }

    float axisPercent;
    if (withDeadbands) {
        if (valueNormalized > calibration.deadband) {
            axisPercent = (valueNormalized - calibration.deadband) / (axisLength - calibration.deadband);
        }
        else if (valueNormalized < -calibration.deadband) {
            axisPercent = (valueNormalized + calibration.deadband) / (axisLength - calibration.deadband);
        }
        else {
            axisPercent = 0.f;
        }
    }
    else {
        axisPercent = valueNormalized / axisLength;
    }

    float correctedValue = axisBasis * axisPercent;
    if (calibration.reversed) {
        correctedValue *= -1.0f;
    }

    return std::max(-1.0f, std::min(correctedValue, 1.0f));
}

//------------------------------------------------------------
// Procedure: _handleButtons()

void Joystick::_handleButtons()
{
    if (m_pollingFor == NotPolling) {
        std::cerr << "Internal Error: Joystick not polling!" << std::endl;
        return;
    }

    std::cout << "tutto il giorno sopra i libri ora voglio stare easy cazzo mene della crisi faccio solo la 6 litri" << std::endl; 
    _updateButtonEventStates(_buttonEventStates);
    std::cout << "sboccing like no tomorrow and" << std::endl;

    if (m_pollingFor == PollingForConfiguration) {
        for (int buttonIndex = 0; buttonIndex < m_numButtons; buttonIndex++) {
            if (_buttonEventStates[buttonIndex] == ButtonEventDownTransition) {
                std::cout << "Button pressed - button" << buttonIndex << std::endl;
                std::string str = "BUTTON_" + std::to_string(buttonIndex);
                Notify(str, true);
            }
            else if (_buttonEventStates[buttonIndex] == ButtonEventUpTransition) {
                std::cout << "Button released - button" << buttonIndex << std::endl;
                std::string str = "BUTTON_" + std::to_string(buttonIndex);
                Notify(str, false);
            }
        }
    }
    else if (m_pollingFor == PollingForVehicle) {
        //-- Process button press/release
        const int buttonDelay = static_cast<int>(1000.0 / m_buttonFreq); //ogni quanto ripeto azione mentre il bottone è premuto
        Uint64 start = SDL_GetPerformanceCounter();

        for (int buttonIndex = 0; buttonIndex < (m_numButtons + (4 * m_numHats)); buttonIndex++) {
            if (_assignedButtonActions[buttonIndex].assigned == false) { //vettore con associata una funzione per ogni tasto
                continue;
            }
            auto& assignedAction = _assignedButtonActions[buttonIndex]; //da JoystickConfig.cpp -> _loadButtonSettings

            const std::string buttonAction = assignedAction.actionName;
            if (buttonAction.empty() || (buttonAction == "_buttonActionNone")) {
                continue;
            }

            auto buttonEventState = _buttonEventStates[buttonIndex];
            if (buttonEventState == ButtonEventDownTransition || buttonEventState == ButtonEventRepeat) { //se premo il bottone o conitnuo a premerlo
                if (assignedAction.repeat) { //'questa azione è progettata per essere eseguita continuamente mentre è premuto il bottone?'
                    double elapsedMs = (SDL_GetPerformanceCounter() - start) * 1000.0 / SDL_GetPerformanceFrequency(); //SDL_GetPerformanceFrequency, numero di tick in un secondo
                    if (elapsedMs > buttonDelay) {
                        assignedAction.time = SDL_GetPerformanceCounter(); //ho eseguito l'azione, aggiorno il contatore
                        _executeButtonAction(buttonAction, ButtonEventRepeat);
                    }
                }
                else {
                    if (buttonEventState == ButtonEventDownTransition) { //azione per cui non è previsto che il bottone possa rimanere premuto
                        // (Check for multi-button action)
                        _executeButtonAction(buttonAction, ButtonEventDownTransition);
                        return;
                    }
                }
            }
            else if (buttonEventState == ButtonEventUpTransition) {
                _executeButtonAction(buttonAction, ButtonEventUpTransition);
                return;
            }
        }
    }
}

//------------------------------------------------------------
// helper functions for _handleButtons()

bool Joystick::_getButton(int idx) const
{
    // First try the standardized gamepad set if idx is inside that set
    if (m_gameController && (idx >= 0) && (idx < m_numButtons)) {
        return SDL_GameControllerGetButton(m_gameController, static_cast<SDL_GameControllerButton>(idx));
    }

    // Fall back to raw joystick buttons (covers unmapped/extras)
    if (m_joystick && (idx >= 0) && (idx < m_numButtons)) {
        return SDL_JoystickGetButton(m_joystick, idx);
    }

    return false;
}

void Joystick::_updateButtonEventState(bool buttonPressed, ButtonEvent_t& buttonEventState)
{
    if (buttonPressed) {
        if (buttonEventState == ButtonEventNone) {
            buttonEventState = ButtonEventDownTransition;
        }
        else if (buttonEventState == ButtonEventDownTransition) {
            buttonEventState = ButtonEventRepeat;
        }
    }
    else {
        if (buttonEventState == ButtonEventDownTransition || buttonEventState == ButtonEventRepeat) {
            buttonEventState = ButtonEventUpTransition; //il bottone era premuto ed è stato rilasciato
        }
        else if (buttonEventState == ButtonEventUpTransition) {
            buttonEventState = ButtonEventNone; //il bottone continua a non essere premuto
        }
    }
}

bool Joystick::_getHat(int hat, int idx) const
{
    static constexpr std::array<uint8_t, 4> hatButtons = { SDL_HAT_UP, SDL_HAT_DOWN, SDL_HAT_LEFT, SDL_HAT_RIGHT }; //sono attive solo queste direzioni

    if (idx < 0 || static_cast<size_t>(idx) >= hatButtons.size()) {
        return false;
    }

    return ((SDL_JoystickGetHat(m_joystick, hat) & hatButtons[idx]) != 0); //restituisce il valore del tasto dell'hat, solo per il joystick (nel game controller sono considerati bottoni)
    //confronta il valore di SDL_JoystickGetHat con il valore del tasto corrente per capire se attivo
}

void Joystick::_updateButtonEventStates(std::vector<ButtonEvent_t>& buttonEventStates)
{
    if (buttonEventStates.size() > (m_numButtons + (4 * m_numHats))) {
        std::cerr << "Internal Error: buttonEventStates size incorrect!" << std::endl;
        return;
    }

    // Update standard buttons - index 0 to buttonCount-1
    for (int buttonIndex = 0; buttonIndex < m_numButtons; buttonIndex++) {
        const bool buttonIsPressed = _getButton(buttonIndex); //con _getButton ottengo il booleano per il tasto
        _updateButtonEventState(buttonIsPressed, buttonEventStates[buttonIndex]); //aggiorno il ButtonEventStates con il valore corrente tenendo conto dello stato del tasto
    }

    // Update hat buttons - indexes buttonCount to totalButtonCount-1
    const int numHatButtons = 4;
    int nextIndex = m_numButtons;
    for (int hatIndex = 0; hatIndex < m_numHats; hatIndex++) {
        for (int hatButtonIndex = 0; hatButtonIndex < numHatButtons; hatButtonIndex++) {
            const bool buttonIsPressed = _getHat(hatIndex, hatButtonIndex);
            const int currentIndex = nextIndex; //l'indice corrente diventa il prossimo
            _updateButtonEventState(buttonIsPressed, buttonEventStates[currentIndex]);
            ++nextIndex; //l'indice prossimo viene incrementato, alla prossima iterazione del 'for' l'indice corrente viene incrementato
        }
    }
}

void Joystick::_executeButtonAction(const std::string& action, ButtonEvent_t buttonEvent)
{
    if (action == _buttonActionNone) {
        return;
    }

    struct ActionInfo {
        const std::string action;
        const ButtonEvent_t event;
        std::function<void()> func; //contenitore di funzione senza argomenti e senza return
    };
    //array di ActionInfo, per righe:
    auto actionInfo = std::to_array<ActionInfo>({
        { _buttonActionArm,                     ButtonEventDownTransition,  [this]() { Notify("setArmed", true); } }, //this: istanza della classe a cui si sta lavorando
        { _buttonActionDisarm,                  ButtonEventDownTransition,  [this]() { Notify("setArmed", false); } },
        //{ _buttonActionToggleArm,               ButtonEventDownTransition,  [this, vehicle]() { emit setArmed(!vehicle->armed()); } },
        { _buttonActionVTOLFixedWing,           ButtonEventDownTransition,  [this]() { Notify("setVtolInFwdFlight", true); } },
        { _buttonActionVTOLMultiRotor,          ButtonEventDownTransition,  [this]() { Notify("setVtolInFwdFlight", false); } },
        { _buttonActionTriggerCamera,           ButtonEventDownTransition,  [this]() { Notify("triggerCamera", 1); } },
        { _buttonActionContinuousZoomIn,        ButtonEventDownTransition,  [this]() { Notify("startContinuousZoom", 1); } },
        { _buttonActionContinuousZoomIn,        ButtonEventRepeat,          [this]() { Notify("startContinuousZoom", 1); } },
        { _buttonActionContinuousZoomOut,       ButtonEventDownTransition,  [this]() { Notify("startContinuousZoom", -1); } },
        { _buttonActionContinuousZoomOut,       ButtonEventRepeat,          [this]() { Notify("startContinuousZoom", -1); } },
        { _buttonActionStepZoomIn,              ButtonEventDownTransition,  [this]() { Notify("stepZoom", 1); } },
        { _buttonActionStepZoomIn,              ButtonEventRepeat,          [this]() { Notify("stepZoom", 1); } },
        { _buttonActionStepZoomOut,             ButtonEventDownTransition,  [this]() { Notify("stepZoom", -1); } },
        { _buttonActionStepZoomOut,             ButtonEventRepeat,          [this]() { Notify("stepZoom", -1); } },
        { _buttonActionNextStream,              ButtonEventDownTransition,  [this]() { Notify("stepStream", 1); } },
        { _buttonActionPreviousStream,          ButtonEventDownTransition,  [this]() { Notify("stepStream", -1); } },
        { _buttonActionNextCamera,              ButtonEventDownTransition,  [this]() { Notify("stepCamera", 1); } },
        { _buttonActionPreviousCamera,          ButtonEventDownTransition,  [this]() { Notify("stepCamera", -1); } },
        { _buttonActionGimbalUp,                ButtonEventDownTransition,  [this]() { Notify("gimbalPitchStart", 1); } },
        { _buttonActionGimbalUp,                ButtonEventUpTransition,    [this]() { Notify("gimbalPitchStop", 1); } },
        { _buttonActionGimbalDown,              ButtonEventDownTransition,  [this]() { Notify("gimbalPitchStart", -1); } },
        { _buttonActionGimbalDown,              ButtonEventUpTransition,    [this]() { Notify("gimbalPitchStop", 1); } },
        { _buttonActionGimbalLeft,              ButtonEventDownTransition,  [this]() { Notify("gimbalYawStart", -1); } },
        { _buttonActionGimbalLeft,              ButtonEventUpTransition,    [this]() { Notify("gimbalYawStop", 1); } },
        { _buttonActionGimbalRight,             ButtonEventDownTransition,  [this]() { Notify("gimbalYawStart", 1); } },
        { _buttonActionGimbalRight,             ButtonEventUpTransition,    [this]() { Notify("gimbalYawStop", 1); } },
        { _buttonActionStartVideoRecord,        ButtonEventDownTransition,  [this]() { Notify("startVideoRecord", 1); } },
        { _buttonActionStopVideoRecord,         ButtonEventDownTransition,  [this]() { Notify("stopVideoRecord", 1); } },
        { _buttonActionToggleVideoRecord,       ButtonEventDownTransition,  [this]() { Notify("toggleVideoRecord", 1); } },
        { _buttonActionGimbalCenter,            ButtonEventDownTransition,  [this]() { Notify("centerGimbal", 1); } },
        { _buttonActionGimbalYawLock,           ButtonEventDownTransition,  [this]() { Notify("gimbalYawLock", true); } },
        { _buttonActionGimbalYawFollow,         ButtonEventDownTransition,  [this]() { Notify("gimbalYawLock", false); } },
        { _buttonActionEmergencyStop,           ButtonEventDownTransition,  [this]() { Notify("emergencyStop", 1); } },
        //{ _buttonActionGripperGrab,             ButtonEventDownTransition,  [this]() { emit gripperAction(GRIPPER_ACTION_GRAB); } },
        //{ _buttonActionGripperRelease,          ButtonEventDownTransition,  [this]() { emit gripperAction(GRIPPER_ACTION_RELEASE); } },
        //{ _buttonActionGripperHold,             ButtonEventDownTransition,  [this]() { emit gripperAction(GRIPPER_ACTION_HOLD); } },
        //{ _buttonActionLandingGearDeploy,       ButtonEventDownTransition,  [this]() { emit landingGearDeploy(); } },
        //{ _buttonActionLandingGearRetract,      ButtonEventDownTransition,  [this]() { emit landingGearRetract(); } },
        { _buttonActionMotorInterlockEnable,    ButtonEventDownTransition,  [this]() { Notify("motorInterlock", true); } },
        { _buttonActionMotorInterlockDisable,   ButtonEventDownTransition,  [this]() { Notify("motorInterlock", false); } },
        });

    // Now look for an action match
    auto it = std::find_if(actionInfo.begin(), actionInfo.end(), [&action, &buttonEvent](const ActionInfo& info) { //lambda function che usa variabili esterne specificate tra '[]'
        return (info.action == action) && (info.event == buttonEvent);
        }); //cerca il primo elemento di actionInfo tale che corrisponda sia come action che come buttonEvent
    if (it != actionInfo.end()) {
        it->func(); //se diverso dalla fine dell'array, esegue la funzione
        return;
    }

    // Finally let mavlink actions have a go
    //if (buttonEvent == ButtonEventDownTransition) {
    //    emit unknownAction(action);
    //    for (int i = 0; i < _mavlinkActionManager->actions()->count(); i++) {
    //        MavlinkAction* const mavlinkAction = _mavlinkActionManager->actions()->value<MavlinkAction*>(i);
    //        if (action == mavlinkAction->label()) {
    //            mavlinkAction->sendTo(vehicle);
    //            return;
    //        }
    //    }
    //}
}
