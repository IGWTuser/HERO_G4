/**
 * @file DetectorConstruction.cc
 * @brief Реализация конструкции детектора для симуляции
 *
 * Определяет геометрию детектора HERO:
 * - Мир (World) - объем, содержащий всю геометрию
 * - Детектор - регулярная шестиугольная призма с композиционным материалом
 *
 * **Геометрия HERO:**
 * ```
 * Вид сверху (в плоскости XY):
 *
 *         Z
 *         ↑
 *    ◇────────◇
 *   ╱          ╲
 *  ◇            ◇  ← Регулярный шестиугольник
 *  │     Y      │    (правильный 6-угольник)
 *  │     ↑      │    Радиус: 800 мм
 *  │     └──X   │    Высота: 1470 мм
 *  ◇            ◇
 *   ╲          ╱
 *    ◇────────◇
 * ```
 *
 * **Материалы:**
 * - Вакуум (G4_Galactic) - мир и внутри детектора
 * - Композит (смесь полистирена и вольфрама) - детектор
 *   - 21.5% полистирена (scintillator)
 *   - 78.5% вольфрама (heavy material для поглощения)
 *   - Плотность: 4.1 г/см³
 *
 * **Единицы GEANT4:**
 * - Длины: mm, cm, m
 * - Материалы: г/см³ (g/cm3)
 * - Углы: deg, rad
 *
 * @date 2024
 * @version 1.0
 *
 * @note Используется G4Polyhedra для создания правильного многоугольника
 * @note Материалы из базы данных NIST (G4NistManager)
 * @see G4VUserDetectorConstruction для базового класса
 */

#include "DetectorConstruction.hh"

#include "G4NistManager.hh"
#include "G4Material.hh"
#include "G4Polyhedra.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"

/**
 * @brief Конструктор по умолчанию
 *
 * Инициализирует DetectorConstruction.
 * Наследуется из G4VUserDetectorConstruction.
 *
 * @note Конструктор обычно пуст - вся инициализация в Construct()
 */
DetectorConstruction::DetectorConstruction() : G4VUserDetectorConstruction() { }

/**
 * @brief Деструктор
 *
 * Освобождает ресурсы при удалении объекта.
 * Память геометрии управляется GEANT4.
 *
 * @note Деструктор обычно пуст - GEANT4 управляет памятью
 */
DetectorConstruction::~DetectorConstruction() { }

/**
 * @brief Построить геометрию детектора
 *
 * Главный метод для создания геометрии симуляции.
 * Вызывается один раз при инициализации RunManager.
 *
 * **Структура геометрии:**
 * ```
 * Мир (World)
 * └── Детектор (Detector)
 *     Регулярный шестиугольник
 *     Материал: полистирен + вольфрам
 * ```
 *
 * **Процесс создания геометрии в GEANT4:**
 * 1. **Материалы** - определить материалы
 * 2. **Solid** - создать геометрическую форму (G4Box, G4Polyhedra и т.д.)
 * 3. **LogicalVolume** - связать форму с материалом
 * 4. **PhysicalVolume** - разместить в пространстве (G4PVPlacement)
 * 5. **Визуализация** - добавить цвет и атрибуты (G4VisAttributes)
 *
 * **Возвращаемое значение:**
 * Указатель на физический объем "Мира" (World).
 * Это корневой объем, содержащий всю геометрию.
 *
 * @return G4VPhysicalVolume* Указатель на физический объем мира
 *
 * @see G4NistManager - менеджер материалов NIST
 * @see G4Polyhedra - класс для создания многоугольных призм
 * @see G4PVPlacement - размещение объемов в пространстве
 */
G4VPhysicalVolume* DetectorConstruction::Construct() {
    // ========== 1. МАТЕРИАЛЫ ==========
    // Получаем менеджер материалов NIST
    // NIST имеет предопределённые материалы: G4_Al, G4_Fe, G4_W и т.д.
    G4NistManager* nist = G4NistManager::Instance();
    
    // Получаем вакуум из базы NIST
    // G4_Galactic - специальный материал GEANT4 для "пустого пространства"
    // Плотность очень низкая (примерно 10^-25 г/см³)
    G4Material* vacuum = nist->FindOrBuildMaterial("G4_Galactic");
    
    // ========== 2. МИР (WORLD) ==========
    // Мир - это объем, который содержит всю геометрию
    // Должен быть достаточно большой, чтобы вместить всё
    // Все частицы, выходящие за пределы мира, удаляются из симуляции
    
    G4double worldSize = 5.0 * m;  // 5 метров
    
    // Создаём solid (геометрическую форму) - куб
    G4Box* solidWorld = new G4Box("World",                    // Название
                                   0.5 * worldSize,           // Half-size X
                                   0.5 * worldSize,           // Half-size Y
                                   0.5 * worldSize);          // Half-size Z
    
    // Создаём логический объем - связываем форму с материалом
    G4LogicalVolume* logicWorld = new G4LogicalVolume(solidWorld,    // Форма
                                                       vacuum,        // Материал
                                                       "World");      // Название
    
    // Размещаем мир в пространстве
    // Это корневой объем - указываем nullptr для mother volume
    G4VPhysicalVolume* physWorld = new G4PVPlacement(0,                    // Ротация
                                                      G4ThreeVector(),     // Позиция (0,0,0)
                                                      logicWorld,          // Логический объем
                                                      "World",             // Название
                                                      0,                   // Материнский объем (nullptr для корня)
                                                      false,               // Не делим копии (no division)
                                                      0,                   // Номер копии
                                                      true);               // Проверять перекрытия
    
    // ========== 3. МАТЕРИАЛЫ ДЕТЕКТОРА ==========
    // Детектор состоит из смеси:
    // - Полистирена (scintillator) - светит при проходе частицы
    // - Вольфрама (heavy material) - поглощает энергию
    
    // Получаем материалы из NIST
    G4Material* polystyrene = nist->FindOrBuildMaterial("G4_POLYSTYRENE");
    G4Material* tungsten = nist->FindOrBuildMaterial("G4_W");
    
    // Создаём новый материал как смесь
    G4double compositeDensity = 4.1 * g/cm3;  // Плотность смеси
    G4Material* compMaterial = new G4Material("CompMat",      // Название
                                               compositeDensity,  // Плотность
                                               2);                // Количество компонентов
    
    // Добавляем компоненты по массовой доле
    // Вся доля должна составлять 1.0 (100%)
    compMaterial->AddMaterial(polystyrene, 0.215);  // 21.5% полистирена
    compMaterial->AddMaterial(tungsten,    0.785);  // 78.5% вольфрама
    
    // ========== 4. ДЕТЕКТОР (HEXAGONAL PRISM) ==========
    // Детектор - это правильная шестиугольная призма
    // Используем G4Polyhedra для создания многоугольной формы
    
    // Параметры детектора
    G4double height = 1470 * mm;               // Высота призмы
    G4double zLow = -0.5 * height;             // Z координата снизу
    G4double zHigh = 0.5 * height;             // Z координата сверху
    G4int numSides = 6;                        // Правильный шестиугольник
    G4double rInner = 0;                       // Внутренний радиус (нет выреза в центре)
    G4double rOuter = 800 * mm;                // Радиус описанной окружности
    
    // Параметры для G4Polyhedra
    // Polyhedra создаёт призму с основанием правильного многоугольника
    G4int numZPlanes = 2;                      // 2 плоскости: снизу и сверху
    G4double zPlane[2] = { zLow, zHigh };      // Z координаты плоскостей
    G4double rInnerArr[2] = { rInner, rInner };  // Внутренние радиусы
    G4double rOuterArr[2] = { rOuter, rOuter };  // Внешние радиусы
    
    // Создаём solid детектора
    G4Polyhedra* solidDetector = new G4Polyhedra("Detector",          // Название
                                                  0.*deg,              // Стартовый угол (0°)
                                                  360.*deg,            // Полный угол (360°)
                                                  numSides,            // Количество сторон (6)
                                                  numZPlanes,          // Количество Z плоскостей (2)
                                                  zPlane,              // Массив Z координат
                                                  rInnerArr,           // Массив внутренних радиусов
                                                  rOuterArr);          // Массив внешних радиусов
    
    // Создаём логический объем детектора
    G4LogicalVolume* logicDetector = new G4LogicalVolume(solidDetector,    // Форма
                                                         compMaterial,     // Материал
                                                         "Detector");      // Название
    
    // ========== 5. РАЗМЕЩЕНИЕ ДЕТЕКТОРА В МИРЕ ==========
    // Размещаем детектор в центре мира (0, 0, 0)
    new G4PVPlacement(0,                    // Ротация (нет ротации)
                      G4ThreeVector(),      // Позиция (центр мира)
                      logicDetector,        // Логический объем детектора
                      "Detector",           // Название
                      logicWorld,           // Материнский объем (мир)
                      false,                // Не делим копии
                      0,                    // Номер копии
                      true);                // Проверять перекрытия
    
    // ========== 6. ВИЗУАЛИЗАЦИЯ ==========
    // Добавляем визуальные атрибуты для красивого отображения
    // Это не влияет на физику, только на визуализацию
    
    // Создаём атрибуты: синий цвет (R=0.5, G=0.5, B=1.0)
    G4VisAttributes* visAttr = new G4VisAttributes(G4Colour(0.5, 0.5, 1.0));
    visAttr->SetForceSolid(true);  // Показывать как твёрдый объект (не прозрачный)
    
    // Применяем атрибуты к детектору
    logicDetector->SetVisAttributes(visAttr);
    
    // ========== 7. ВОЗВРАЩАЕМ МИР ==========
    // Возвращаем указатель на физический объем мира
    // Это служит корневым объемом для всей геометрии
    return physWorld;
}