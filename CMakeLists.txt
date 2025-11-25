cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
project(HERO_G4)

# Найти Geant4 и загрузить настройки
find_package(Geant4 REQUIRED ui_all vis_all)
include(${Geant4_USE_FILE})

# Указываем директорию с заголовочными файлами
include_directories(${PROJECT_SOURCE_DIR}/include)

# Собираем список всех исходников
file(GLOB sources 
    ${PROJECT_SOURCE_DIR}/src/*.cc
)

# Собираем список всех заголовочных файлов
file(GLOB headers 
    ${PROJECT_SOURCE_DIR}/include/*.hh
)

# Создаём исполняемый файл
add_executable(HERO_G4 
    main.cc
    ${sources} 
    ${headers}
)

# Линкуем с библиотеками Geant4
target_link_libraries(HERO_G4 ${Geant4_LIBRARIES})

# Копируем макросы в build директорию (если есть)
file(GLOB MACRO_FILES "*.mac")
file(COPY ${MACRO_FILES} DESTINATION ${PROJECT_BINARY_DIR})

# Устанавливаем стандарт C++ (рекомендуется для новых версий Geant4)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
