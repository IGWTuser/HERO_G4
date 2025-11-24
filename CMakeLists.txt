cmake_minimum_required(VERSION 3.8...3.16)
project(MyGeant4Project)

# Поиск пакета GEANT4 с необходимыми компонентами
find_package(Geant4 REQUIRED ui_all vis_all)

include(${Geant4_USE_FILE})

# Указываем список исходных файлов проекта
add_executable(MyGeant4Executable
main.cpp
DetectorConstruction.cc
PrimaryGeneratorAction.cc
MySteppingAction.cc
MyEventAction.cc
)

# Линковка с библиотеками GEANT4
target_link_libraries(MyGeant4Executable ${Geant4_LIBRARIES})
