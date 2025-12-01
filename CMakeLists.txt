cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
project(HERO_G4)

# Ищем и подключаем Geant4
find_package(Geant4 REQUIRED ui_all vis_all)
include(${Geant4_USE_FILE})

# Указываем директорию с заголовочными файлами
include_directories(${PROJECT_SOURCE_DIR}/include)

# Собираем все .cc файлы из папки src
file(GLOB sources 
    ${PROJECT_SOURCE_DIR}/src/*.cc
)

# Собираем все .hh файлы из папки include
file(GLOB headers 
    ${PROJECT_SOURCE_DIR}/include/*.hh
)

# Создаём исполняемый файл из main.cc и всех остальных файлов
add_executable(HERO_G4 
    main.cc
    ${sources} 
    ${headers}
)

# Линкуем с библиотеками Geant4
target_link_libraries(HERO_G4 ${Geant4_LIBRARIES})

# Копируем макросы в build директорию (если есть .mac файлы)
file(GLOB MACRO_FILES "*.mac")
file(COPY ${MACRO_FILES} DESTINATION ${PROJECT_BINARY_DIR})

# Устанавливаем стандарт C++14 (требуется для Geant4)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
