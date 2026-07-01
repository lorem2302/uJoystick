/************************************************************/
/*    NAME:                                               */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: Joystick.h                                          */
/*    DATE: December 29th, 1963                             */
/************************************************************/

#ifndef Joystick_HEADER
#define Joystick_HEADER

#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"

#include "SDL.h"
#include "SDL_joystick.h"
#include "SDL_gamecontroller.h"

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class Joystick : public AppCastingMOOSApp {
public:
  Joystick();
  ~Joystick();

protected: // Standard MOOSApp functions to overload
  bool OnNewMail(MOOSMSG_LIST &NewMail);
  bool Iterate();
  bool OnConnectToServer();
  bool OnStartUp();

protected: // Standard AppCastingMOOSApp function to overload
  bool buildReport();
  void registerVariables();

private:
  static constexpr int AXIS_MIN = -32768;
  static constexpr int AXIS_MAX = 32767;
  static constexpr int AXIS_NOT_SET = -1;

  enum AxisFunction_t {
    rollFunction,
    pitchFunction,
    yawFunction,
    throttleFunction,
    pitchExtensionFunction,
    rollExtensionFunction,
    aux1ExtensionFunction,
    aux2ExtensionFunction,
    aux3ExtensionFunction,
    aux4ExtensionFunction,
    aux5ExtensionFunction,
    aux6ExtensionFunction,
    maxAxisFunction
  };

  enum ButtonEvent_t {
    ButtonEventUpTransition,
    ButtonEventDownTransition,
    ButtonEventRepeat,
    ButtonEventNone
  };

  enum PollingType_t {
    NotPolling,
    PollingForConfiguration,
    PollingForVehicle
  };

  struct AxisCalibration_t {
    int min = AXIS_MIN;
    int max = AXIS_MAX;
    int center = 0;
    int deadband = 0;
    bool reversed = false;
    std::string function2;
    bool* flag = nullptr;

    void reset() {
      min = AXIS_MIN;
      max = AXIS_MAX;
      center = 0;
      deadband = 0;
      reversed = false;
    }
  };

  struct AssignedButtonAction {
    std::string actionName = "_buttonActionNone";
    std::string actionName2 = "_buttonActionNone";
    bool* action2 = nullptr;
    bool assigned = false;
    bool repeat = false;
    uint64_t time = 0;
  };

private: // Configuration variables
    bool m_deadband;
    bool m_throttleModeCenterZero;
    bool m_negativeThrust;
    bool m_circleCorrection;
    bool m_throttleSmoothing;
    double m_exponentialPercent;

    PollingType_t m_pollingFor;

    double m_axisFreq;
    double m_buttonFreq;

    std::vector<AxisCalibration_t> m_axisCalibration;
    
private: // Joystick variables
    bool m_rollPitchToggle = false;
    //bool m_cameraTiltDown = false;
    //bool m_cameraTiltUp = false;
    bool m_depthHoldMode = false;
    bool m_shift = false;
    bool m_manualMode = false;
    bool m_stabilizeMode = false;
    //bool m_increaseGain = false;
    //bool m_lightsBrighter = false;
    //bool m_lightsDimmer = false;
    //bool m_decreaseGain = false;
    //bool m_trimPitchForward = false;
    //bool m_trimPitchBackward = false;
    //bool m_trimRollLeft = false;
    //bool m_trimRollRight = false;
    bool m_cameraTiltCenter = false;
    bool m_toggleInputHold = false;

private: // Mappings
    std::unordered_map<AxisFunction_t, int> m_axisMap;
    std::vector<AssignedButtonAction> _assignedButtonActions; //mappatura bottoni-azione
    
    std::unordered_map<std::string, bool*> m_stateMap; //mappatura nome-puntatore a relativa variabile

private: // SDL / device state
  SDL_Joystick* m_joystick;
  SDL_GameController* m_gameController;
  
  int m_jsIndex; //indice associato al joystick attuale

  char m_GUID[33] = { 0 };
  std::string m_name;
  char* m_gc_mapping;
  
  int m_numAxes = 0;
  int m_numButtons = 0;
  int m_numHats = 0;
  int m_numTrackballs = 0;
  
  std::vector<ButtonEvent_t> _buttonEventStates; //vettore con lo stato di ciascun bottone

private: // Internal helpers
  bool _validAxis(int axis) const;
  //void _setJoystickAxisForAxisFunction(AxisFunction_t axisFunction, int joystickAxis);
  //void _resetFunctionToAxisMap();
  //void _resetAxisCalibrationData();
  //void _resetButtonActionData();
  void _resetButtonEventStates();
  bool _loadButtonSettings(const std::string& settings);

  void _handleAxis();
  void _handleButtons();

  int _getAxisValue(int idx) const;
  int _getJoystickAxisForAxisFunction(AxisFunction_t axisFunction) const;
  float _adjustRange(int value, const AxisCalibration_t& calibration, bool withDeadbands) const;

  bool _getButton(int idx) const;
  bool _getHat(int hat, int idx) const;
  void _updateButtonEventState(bool buttonPressed, ButtonEvent_t& buttonEventState);
  void _updateButtonEventStates(std::vector<ButtonEvent_t>& buttonEventStates);
  void _executeButtonAction(const std::string& action, ButtonEvent_t buttonEvent);

};

#endif
