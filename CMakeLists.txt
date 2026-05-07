#--------------------------------------------------------
# The CMakeLists.txt for:                       uJoystick
# Author(s):                              
#--------------------------------------------------------
find_package(SDL2 REQUIRED)

SET(SRC
  Joystick.cpp
  Joystick_Info.cpp
  main.cpp
  JoystickConfig.cpp
  JoystickUtils.cpp
)

ADD_EXECUTABLE(uJoystick ${SRC})

TARGET_INCLUDE_DIRECTORIES(uJoystick PRIVATE
  ${SDL2_INCLUDE_DIRS}
  #SDL2::SDL2
)

TARGET_LINK_LIBRARIES(uJoystick
   ${MOOS_LIBRARIES}
   apputil
   mbutil
   m
   pthread
   ${SDL2_LIBRARIES}
   )

