/************************************************************/
/*    NAME:                                                 */
/*    ORGN:                                                 */
/*    FILE: Joystick.cpp                                    */
/*    DATE: May 04th, 2026                                  */
/************************************************************/

#include <iterator>
#include "MBUtils.h"
#include "ACTable.h"

#include "Joystick.h"
#include "JoystickUtils.h"
#include "JoystickConfig.h"

using namespace std;

#define STD_NUM_AXES 4
#define STD_NUM_BUTTONS 14

//---------------------------------------------------------
// Constructor()

Joystick::Joystick()
{
    //caratteristiche del joystick
    m_joystick = nullptr;
    m_gameController = nullptr;
    
    //m_GUID = { 0 }; //deve essere un char di 33 elementi
    m_name = { 0 }; //[char]
    m_numAxes = 0;
    m_numButtons = 0;
    m_numHats = 0;
    m_numTrackballs = 0;

    //parametri di configurazione
    m_deadband = false;
    m_throttleModeCenterZero = false;
    m_negativeThrust = false;
    m_circleCorrection = false;
    m_throttleSmoothing = false;
    m_exponentialPercent = 0.0;

    m_axisCalibration.resize(STD_NUM_AXES);
    _buttonEventStates.resize(STD_NUM_BUTTONS);
    _assignedButtonActions.resize(STD_NUM_BUTTONS);

    m_axisFreq = 1000/20; //aggiornamento assi ogni 20 ms
    m_buttonFreq = 1000/50; //ripetizione dell'azione ogni 50 ms se si tiene premuto il bottone

    //mappa degli assi
    //m_axisMap inizializzato dopo

    //stato
    m_pollingFor = NotPolling;

}

//---------------------------------------------------------
// Destructor

Joystick::~Joystick()
{
    if (SDL_IsGameController(m_jsIndex) == SDL_TRUE) {
        SDL_free(m_gc_mapping);
        SDL_GameControllerClose(m_gameController);
    }
    else {
        SDL_JoystickClose(m_joystick);
    }
    SDL_Quit();
}

//---------------------------------------------------------
// Procedure: OnNewMail()

bool Joystick::OnNewMail(MOOSMSG_LIST &NewMail)
{
  AppCastingMOOSApp::OnNewMail(NewMail);
  //la MOOS app è di sola pubblicazione, non si iscrive a nessuna variabile
  MOOSMSG_LIST::iterator p;
  for(p=NewMail.begin(); p!=NewMail.end(); p++) {
    CMOOSMsg &msg = *p;
    string key    = msg.GetKey();

#if 0 // Keep these around just for template
    string comm  = msg.GetCommunity();
    double dval  = msg.GetDouble();
    string sval  = msg.GetString(); 
    string msrc  = msg.GetSource();
    double mtime = msg.GetTime();
    bool   mdbl  = msg.IsDouble();
    bool   mstr  = msg.IsString();
#endif

     if(key != "APPCAST_REQ") // handled by AppCastingMOOSApp
       reportRunWarning("Unhandled Mail: " + key);
   }
	
   return(true);
}

//---------------------------------------------------------
// Procedure: OnConnectToServer()

bool Joystick::OnConnectToServer()
{
   registerVariables();
   return(true);
}

//---------------------------------------------------------
// Procedure: Iterate()
//            happens AppTick times per second

bool Joystick::Iterate()
{
  AppCastingMOOSApp::Iterate();
  // Polling logic delegated to JoystickUtils.cpp
  
  // SDL internal state refresh.
  if (SDL_IsGameController(m_jsIndex) == SDL_TRUE) {
      if (SDL_GameControllerGetAttached(m_gameController) == SDL_FALSE) {
        cerr << "Game Controller " << m_name << " disconnected" << endl;
        return false;
        }
      }
  else {
      if (SDL_JoystickGetAttached(m_joystick) == SDL_FALSE) {
        cerr << "Joystick " << m_name << " disconnected" << endl;
        return false;
        }
      }
      
  SDL_GameControllerUpdate();

  std::cout << "******************************QUIQUIQUIQUIQUI*************************" << std::endl;

  _handleButtons();
  std::cout << "******************************QUIQUIQUIQUIQUI*************************" << std::endl;
  _handleAxis();

  AppCastingMOOSApp::PostReport();
  return true;
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//            happens before connection is open

bool Joystick::OnStartUp()
{
  AppCastingMOOSApp::OnStartUp();

  STRING_LIST sParams;
  m_MissionReader.EnableVerbatimQuoting(false);
  if(!m_MissionReader.GetConfiguration(GetAppName(), sParams))
    reportConfigWarning("No config block found for " + GetAppName());

  STRING_LIST::iterator p;
  for(p=sParams.begin(); p!=sParams.end(); p++) {
    string orig  = *p;
    string param = stripBlankEnds(toupper(biteString(*p, '=')));
    string value = stripBlankEnds(*p);
    double dval = atof(value.c_str());
    bool   bval = (strcasecmp(value.c_str(), "TRUE") == 0 || dval != 0);

    bool handled = false;
    
    if(param == "POLLING_FOR") {
        if (value == "NOT_POLLING") {
            m_pollingFor = NotPolling;
            handled = true;
        }
        else if (value == "CONFIGURATION") {
            m_pollingFor = PollingForConfiguration;
            handled = true;
        }
        else if (value == "VEHICLE") {
            m_pollingFor = PollingForVehicle;
            handled = true;
        }
        else {
            std::cerr << value << " is not a valid operation mode. The available modes are: NOT_POLLING, CONFIGURATION, VEHICLE" << std::endl;
            handled = false;
        }
    }
    else if(param == "USE_DEADBANDS") {
        m_deadband = bval;
        handled = true;
    }
    else if(param == "THROTTLE_MODE_CENTER_ZERO") {
        m_throttleModeCenterZero = bval;
        handled = true;
    }
    else if (param == "NEGATIVE_THRUST") {
        m_negativeThrust = bval;
        handled = true;
    }
    else if(param == "CIRCLE_CORRECTION") {
        m_circleCorrection = bval;
        handled = true;
    }
    else if (param == "THROTTLE_SMOOTHING") {
        m_throttleSmoothing = bval;
        handled = true;
    }
    else if (param == "EXPONENTIAL_PERCENT") {
        m_exponentialPercent = dval;
        handled = true;
    }

    else if (param == "ROLL_FUNCTION") {
        stringstream ss(value);
        string item;
        int curr_index = -1;

        while (getline(ss, item, ',')) { //un ciclo per ogni elemento di 'ROLL_FUNCTION'
            std::stringstream itemStream(item);

            std::string name, values;
            getline(itemStream, name, ':'); //salva la parte prima dell'uguale in 'name'
            getline(itemStream, values);
            name = stripBlankEnds(toupper(name));
            std::cout << name << std::endl;
            values = stripBlankEnds(values);
            int ivals = atoi(values.c_str());
            bool   bvals = (strcasecmp(values.c_str(), "TRUE") == 0 || dval != 0);
            if (name == "AXIS") {
                m_axisMap.insert({ rollFunction, ivals });
                curr_index = ivals;
                handled = true;
            }
            else if (curr_index >= 0){
                                
                if (name == "MIN") {
                    m_axisCalibration[curr_index].min = ivals;
                    handled &= true;
                }
                else if (name == "MAX") {
                    m_axisCalibration[curr_index].max = ivals;
                    handled &= true;
                }
                else if (name == "CENTER") {
                    m_axisCalibration[curr_index].center = ivals;
                    handled &= true;
                }
                else if (name == "DEADBAND") {
                    m_axisCalibration[curr_index].deadband = ivals;
                    handled &= true;
                }
                else if (name == "REVERSED") {
                    m_axisCalibration[curr_index].reversed = bvals;
                    handled &= true;
                }
            }
            else {
                std::cerr << "Index unavailable" << std::endl;
                handled = false;
                }
        }
    }
    else if (param == "PITCH_FUNCTION") {
        stringstream ss(value);
        string item;
        int curr_index = -1;

        while (getline(ss, item, ',')) { //un ciclo per ogni elemento di 'PITCH_FUNCTION'
            std::stringstream itemStream(item);

            std::string name, values;
            getline(itemStream, name, ':'); //salva la parte prima dell'uguale in 'name'
            getline(itemStream, values);
            name = stripBlankEnds(toupper(name));
            values = stripBlankEnds(values);
            int ivals = atoi(values.c_str());
            bool   bvals = (strcasecmp(values.c_str(), "TRUE") == 0 || dval != 0);
            if (name == "AXIS") {
                m_axisMap.insert({ pitchFunction, ivals });
                curr_index = ivals;
                handled = true;
            }
            else if (curr_index >= 0){
                                
                if (name == "MIN") {
                    m_axisCalibration[curr_index].min = ivals;
                    handled &= true;
                }
                else if (name == "MAX") {
                    m_axisCalibration[curr_index].max = ivals;
                    handled &= true;
                }
                else if (name == "CENTER") {
                    m_axisCalibration[curr_index].center = ivals;
                    handled &= true;
                }
                else if (name == "DEADBAND") {
                    m_axisCalibration[curr_index].deadband = ivals;
                    handled &= true;
                }
                else if (name == "REVERSED") {
                    m_axisCalibration[curr_index].reversed = bvals;
                    handled &= true;
                }
            }
            else {
                std::cerr << "Index unavailable" << std::endl;
                handled = false;
                }
        }
    }
    else if (param == "YAW_FUNCTION") {
        stringstream ss(value);
        string item;
        int curr_index = -1;

        while (getline(ss, item, ',')) { //un ciclo per ogni elemento di 'YAW_FUNCTION'
            std::stringstream itemStream(item);

            std::string name, values;
            getline(itemStream, name, ':'); //salva la parte prima dell'uguale in 'name'
            getline(itemStream, values);
            name = stripBlankEnds(toupper(name));
            std::cout << name << std::endl;
            values = stripBlankEnds(values);
            int ivals = atoi(values.c_str());
            bool   bvals = (strcasecmp(values.c_str(), "TRUE") == 0 || dval != 0);
            if (name == "AXIS") {
                m_axisMap.insert({ yawFunction, ivals });
                curr_index = ivals;
                handled = true;
            }
            else if (curr_index >= 0){
                                
                if (name == "MIN") {
                    m_axisCalibration[curr_index].min = ivals;
                    handled &= true;
                }
                else if (name == "MAX") {
                    m_axisCalibration[curr_index].max = ivals;
                    handled &= true;
                }
                else if (name == "CENTER") {
                    m_axisCalibration[curr_index].center = ivals;
                    handled &= true;
                }
                else if (name == "DEADBAND") {
                    m_axisCalibration[curr_index].deadband = ivals;
                    handled &= true;
                }
                else if (name == "REVERSED") {
                    m_axisCalibration[curr_index].reversed = bvals;
                    handled &= true;
                }
            }
            else {
                std::cerr << "Index unavailable" << std::endl;
                handled = false;
                }
        }
    }
    else if (param == "THROTTLE_FUNCTION") {
        stringstream ss(value);
        string item;
        int curr_index = -1;

        while (getline(ss, item, ',')) { //un ciclo per ogni elemento di 'ROLL_FUNCTION'
            std::stringstream itemStream(item);

            std::string name, values;
            getline(itemStream, name, ':'); //salva la parte prima dell'uguale in 'name'
            getline(itemStream, values);
            name = stripBlankEnds(toupper(name));
            std::cout << name << std::endl;
            values = stripBlankEnds(values);
            int ivals = atoi(values.c_str());
            bool   bvals = (strcasecmp(values.c_str(), "TRUE") == 0 || dval != 0);
            if (name == "AXIS") {
                m_axisMap.insert({ throttleFunction, ivals });
                curr_index = ivals;
                handled = true;
            }
            else if (curr_index >= 0){
                                
                if (name == "MIN") {
                    m_axisCalibration[curr_index].min = ivals;
                    handled &= true;
                }
                else if (name == "MAX") {
                    m_axisCalibration[curr_index].max = ivals;
                    handled &= true;
                }
                else if (name == "CENTER") {
                    m_axisCalibration[curr_index].center = ivals;
                    handled &= true;
                }
                else if (name == "DEADBAND") {
                    m_axisCalibration[curr_index].deadband = ivals;
                    handled &= true;
                }
                else if (name == "REVERSED") {
                    m_axisCalibration[curr_index].reversed = bvals;
                    handled &= true;
                }
            }
            else {
                std::cerr << "Index unavailable" << std::endl;
                handled = false;
                }
        }
    }

    else if (param == "AXIS_FREQ") {
        m_axisFreq = dval;
        handled = true;
    }
    else if (param == "BUTTON_FREQ") {
        m_buttonFreq = dval;
        handled = true;
    }
    else if (param == "BUTTON_SETTINGS") {
        string path = value;
        _loadButtonSettings(path); // Initial configuration delegated to JoystickConfig.cpp
        handled = true;
    }

    if(!handled)
      reportUnhandledConfigWarning(orig);
  }

  // aprire la comunicazione
  if (SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) != 0) {
      std::cerr << "SDL Init failed: " << SDL_GetError() << std::endl;
      return false;
  }
  const int count = SDL_NumJoysticks();
  if (count <= 0) {
      cerr << "No joystick/game controller connected" << endl;
      return false;
  }

  m_jsIndex = 0;

  if (SDL_IsGameController(m_jsIndex) == SDL_TRUE) {
      m_gameController = SDL_GameControllerOpen(m_jsIndex); //SDL_GameController
      if (!m_gameController) { //se ho un puntatore nullo
          std::cerr << "Error opening Game Controller: " << SDL_GetError() << std::endl;
          return false;
      }
      //info sul game controller
      SDL_Joystick* sdl_js = SDL_GameControllerGetJoystick(m_gameController);
      SDL_JoystickGUID guid = SDL_JoystickGetGUID(sdl_js);
      SDL_JoystickGetGUIDString(guid, m_GUID, sizeof(m_GUID));
      m_name = SDL_GameControllerName(m_gameController);
      m_gc_mapping = SDL_GameControllerMapping(m_gameController);

      m_numAxes = SDL_JoystickNumAxes(sdl_js);
      m_numButtons = SDL_JoystickNumButtons(sdl_js);
      m_numHats = SDL_JoystickNumHats(sdl_js);
      m_numTrackballs = SDL_JoystickNumBalls(sdl_js);

      std::cout << "Game controller " << m_name << " connected : GUID = " << m_GUID << std::endl;
      std::cout << "Mapping: " << m_gc_mapping << std::endl;
  }
  else {
      m_joystick = SDL_JoystickOpen(m_jsIndex); //SDL_Joystick
      if (!m_joystick) {
          std::cerr << "Error opening Joystick: " << SDL_GetError() << std::endl;
          return false;
      }
      //info sul joystick
      SDL_JoystickGUID guid = SDL_JoystickGetGUID(m_joystick);
      SDL_JoystickGetGUIDString(guid, m_GUID, sizeof(m_GUID));
      m_name = SDL_JoystickName(m_joystick);
      m_numAxes = SDL_JoystickNumAxes(m_joystick);
      m_numButtons = SDL_JoystickNumButtons(m_joystick);
      m_numHats = SDL_JoystickNumHats(m_joystick);
      m_numTrackballs = SDL_JoystickNumBalls(m_joystick);
      std::cout << "Joystick " << m_name << " connected : GUID = " << m_GUID << std::endl;
      std::cout << "Features: " << m_numAxes << " axes, " << m_numButtons << " buttons, " << m_numHats << " hats, " << m_numTrackballs << " trackballs." << std::endl;
  }

  _buttonEventStates.resize(m_numButtons);
  _resetButtonEventStates();
  
  registerVariables();	
  return(true);
}

//---------------------------------------------------------
// Procedure: registerVariables()

void Joystick::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  // Register("FOOBAR", 0);
}


//------------------------------------------------------------
// Procedure: buildReport()

bool Joystick::buildReport() {
    m_msgs << "============================================" << endl;
    m_msgs << "Joystick: " << (!m_name.empty() ? m_name : "unknown") << endl;
    m_msgs << "GUID:     " << m_GUID << endl;
    m_msgs << "Axes:     " << m_numAxes << endl;
    m_msgs << "Buttons:  " << m_numButtons << endl;
    m_msgs << "Hats:     " << m_numHats << endl;
    m_msgs << "============================================" << endl;

    ACTable actab(2);
    actab << "Key | Value";
    actab.addHeaderLines();
    actab << "AxisFreqHz" << m_axisFreq;
    actab << "BtnFreqHz" << m_buttonFreq;
    m_msgs << actab.getFormattedString();

    return true;
}
