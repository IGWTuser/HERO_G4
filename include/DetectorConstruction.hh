/**
 * @file DetectorConstruction.hh
 * @brief Построение геометрии детектора GEANT4
 *
 * Содержит класс для создания физической геометрии установки:
 * - Определение материалов (железо, свинец, кремний и т.д.)
 * - Построение мира (World volume) с границами
 * - Создание компонентов детектора (мишень, калориметр, магнит)
 * - Регистрация чувствительных объёмов для сбора данных
 *
 * Этот класс вызывается G4RunManager при инициализации симуляции.
 * Геометрия остаётся неизменной во время всего запуска.
 *
 * Структура вашего детектора (вспомогательно):
 * ```
 * World (вакуум, большой объем)
 * └── Detector Components:
 *     ├── Target (железо 5 мм) - для взаимодействия
 *     ├── Calorimeter (свинец 30 см) - для поглощения энергии
 *     ├── Silicon Tracker (кремний 0.5 мм) - для отслеживания
 *     └── Magnets (опционально) - для управления траекториями
 * ```
 *
 * @date 2024
 * @version 1.0
 *
 * @note Геометрия должна соответствовать реальной установке, но распределение материалов используется упрощёенное HERO 
 * @note Материалы берутся из GEANT4 материальной базы данных
 * @see G4VUserDetectorConstruction, MySteppingAction для регистрации данных
 */

#ifndef DetectorConstruction_h
#define DetectorConstruction_h 1

#include "G4VUserDetectorConstruction.hh"

// Форвард деклерации для оптимизации времени компиляции
class G4LogicalVolume;
class G4VPhysicalVolume;
class G4Material;

/**
 * @class DetectorConstruction
 * @brief Конструктор геометрии детектора для GEANT4
 *
 * Наследуется из G4VUserDetectorConstruction. Главный метод:
 * - Construct() - вызывается один раз при инициализации для построения геометрии
 *
 * Отвечает за:
 * 1. **Определение материалов** - создание материалов с физическими свойствами
 *    (плотность, состав, радиационные длины и т.д.)
 * 2. **Построение логических объёмов** - создание геометрических форм с материалами
 * 3. **Размещение физических объёмов** - позиционирование компонентов в пространстве
 * 4. **Регистрация чувствительных объёмов** - отмечание, где нужно собирать данные
 *
 * Пример иерархии объёмов:
 * ```
 * Construct() создаёт:
 * ├── World Volume (большой BOX из вакуума)
 * │   ├── Target Volume (железо)
 * │   │   └── Physical Volume (одна копия в центре)
 * │   ├── Calorimeter Volume (свинец)
 * │   │   └── Physical Volume (одна копия после target)
 * │   └── Silicon Tracker Volume (кремний)
 * │       └── Physical Volumes (несколько копий для слоёв)
 * └── return указатель на World Physical Volume
 * ```
 *
 * **ВАЖНО**: Все логические объёмы должны быть размещены внутри World.
 * GEANT4 отслеживает частицы только внутри World и его дочерних объёмов.
 *
 * @see G4LogicalVolume для создания логических форм
 * @see G4PVPlacement для размещения физических копий
 * @see G4Material для определения материалов
 */
class DetectorConstruction : public G4VUserDetectorConstruction {
public:
    /**
     * @brief Конструктор
     *
     * Обычно пустой. Вся инициализация происходит в методе Construct().
     * Здесь можно инициализировать указатели на nullptr для безопасности.
     *
     * @note Конструктор вызывается один раз при создании объекта
     * @see Construct()
     */
    DetectorConstruction();
    
    /**
     * @brief Деструктор
     *
     * Освобождает память, выделенную для геометрии.
     * ГЕАНТ4 автоматически удаляет логические и физические объёмы,
     * поэтому здесь обычно ничего не нужно делать.
     *
     * @note Деструктор вызывается один раз при удалении объекта
     */
    virtual ~DetectorConstruction();

    /**
     * @brief Построить геометрию детектора
     *
     * Главный метод конструктора. Вызывается один раз при инициализации G4RunManager.
     * Здесь происходит:
     *
     * 1. **Определение материалов** (DefineMaterials или GetMaterial)
     *    - Железо (Fe) для мишени (target)
     *    - Свинец (Pb) для калориметра (calorimeter)
     *    - Кремний (Si) для трекера (tracker)
     *    - Вакуум для мира (world)
     *
     * 2. **Создание логических объёмов** (G4LogicalVolume)
     *    - Форма (BOX, CYLINDER и т.д.) + материал
     *    - Размеры и физические свойства
     *    - Визуальные атрибуты (цвет, прозрачность) для отладки
     *
     * 3. **Размещение физических объёмов** (G4PVPlacement)
     *    - Позиция в пространстве (x, y, z)
     *    - Ориентация (rotation matrix)
     *    - Родитель-объём (всегда внутри World)
     *
     * 4. **Регистрация чувствительных объёмов**
     *    - SetSensitiveDetector() - отмечает, где собирать энергию
     *    - Эти данные будут доступны в MySteppingAction
     *
     * 5. **Возврат указателя на World**
     *    - ГЕАНТ4 начнёт отслеживание от этой точки
     *
     * **Структура типичного Construct():**
     * ```cpp
     * virtual G4VPhysicalVolume* Construct() override {
     *     // 1. Определить материалы
     *     G4Material* fe = G4Material::GetMaterial("G4_Fe");
     *     G4Material* vacuum = G4Material::GetMaterial("G4_Galactic");
     *
     *     // 2. Создать мир (World)
     *     G4Box* worldBox = new G4Box("World", 1*m, 1*m, 2*m);
     *     G4LogicalVolume* worldLV = new G4LogicalVolume(worldBox, vacuum, "WorldLV");
     *     G4VPhysicalVolume* worldPV = new G4PVPlacement(
     *         0, G4ThreeVector(0, 0, 0), worldLV, "World", 0, false, 0);
     *
     *     // 3. Создать мишень (target)
     *     G4Box* targetBox = new G4Box("Target", 10*cm, 10*cm, 0.5*cm);
     *     G4LogicalVolume* targetLV = new G4LogicalVolume(targetBox, fe, "TargetLV");
     *     new G4PVPlacement(0, G4ThreeVector(0, 0, 0*cm), 
     *                       targetLV, "Target", worldLV, false, 0);
     *     targetLV->SetSensitiveDetector(mysensitiveDetector);
     *
     *     // 4. Вернуть мир
     *     return worldPV;
     * }
     * ```
     *
     * @return G4VPhysicalVolume* Указатель на физический объём мира (World)
     *         ГЕАНТ4 начнёт отслеживание частиц от этой точки
     *
     * @note **ВАЖНО**: Все логические объёмы должны быть размещены внутри World
     * @note Вызывается один раз при инициализации (не для каждого события!)
     * @note **THREAD-SAFE**: В многопоточном режиме вызывается один раз перед стартом
     *
     * @see G4Box, G4Tubs для других геометрических форм
     * @see G4LogicalVolume для создания логических объёмов
     * @see G4PVPlacement для размещения физических копий
     * @see G4VisAttributes для визуализации (цвет, стиль)
     * @see G4Material::GetMaterial() для получения стандартных материалов
     *
     * @example Вызов из main:
     * ```cpp
     * DetectorConstruction* detector = new DetectorConstruction();
     * G4RunManager* runManager = new G4RunManager();
     * runManager->SetUserInitialization(detector);
     * // detector->Construct() вызовется автоматически
     * ```
     */
    virtual G4VPhysicalVolume* Construct() override;
};

#endif