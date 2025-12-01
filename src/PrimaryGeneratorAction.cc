/**
 * @file PrimaryGeneratorAction.cc
 * @brief Реализация генератора первичных частиц
 *
 * Отвечает за создание первичной частицы в начале каждого события.
 * Главная обязанность: создать протон/нейтрон с нужной энергией и направлением.
 *
 * **Основные функции:**
 * - Конструктор - инициализирует G4ParticleGun с параметрами
 * - GeneratePrimaries() - создать первичную частицу для события
 * - Setter методы - изменить тип частицы и энергию
 *
 * **Жизненный цикл первичной частицы:**
 * ```
 * main():
 *     ├─ PrimaryGeneratorAction* primGen = new PrimaryGeneratorAction()
 *     │  └─ Создать G4ParticleGun с параметрами
 *     │
 *     └─ runManager->BeamOn(N events)
 *         ↓
 *     Для каждого события:
 *         ├─ GeneratePrimaries(event)
 *         │  ├─ Получить текущие параметры из пушки
 *         │  ├─ Создать первичную частицу
 *         │  └─ Добавить в событие
 *         │
 *         └─ GEANT4 начинает отслеживание этой частицы
 *            ├─ MySteppingAction обрабатывает каждый шаг
 *            └─ MyEventAction анализирует результаты
 * ```
 *
 * **Параметры частицы:**
 * - Тип: proton, neutron, pion и т.д. (G4ParticleDefinition)
 * - Энергия: 10 ГэВ, 100 ГэВ и т.д. (по GEANT4 единицам)
 * - Позиция: где находится "пушка" в пространстве
 * - Направление: куда летит частица
 *
 * **Пример использования в main():**
 * ```cpp
 * PrimaryGeneratorAction* primGen = new PrimaryGeneratorAction();
 *
 * // Стрелять 1000 протонов с энергией 10 ТеВ
 * primGen->SetParticleEnergy(10.0 * TeV);
 * runManager->BeamOn(1000);
 *
 * // Затем стрелять 500 нейтронов с энергией 5 ТеВ
 * primGen->SetParticleType("neutron");
 * primGen->SetParticleEnergy(5.0 * TeV);
 * runManager->BeamOn(500);
 * ```
 * @date 2024
 * @version 1.0
 *
 * @note Используется G4ParticleGun для создания частиц
 * @note Параметры могут изменяться между событиями
 * @note Генерирует ровно 1 частицу за событие (параметр конструктора)
 * @see G4VUserPrimaryGeneratorAction, G4ParticleGun
 */

#include "PrimaryGeneratorAction.hh"

#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4Proton.hh"
#include "G4SystemOfUnits.hh"
#include "G4Event.hh"
#include "G4ThreeVector.hh"
#include "Randomize.hh"

/**
 * @brief Конструктор
 *
 * Инициализирует генератор первичных частиц.
 * Создаёт G4ParticleGun и устанавливает параметры по умолчанию.
 *
 * **Процесс инициализации:**
 * 1. Создать G4ParticleGun(1) - пушка для 1 частицы за событие
 * 2. Установить тип частицы - протон (по умолчанию)
 * 3. Установить позицию пушки - 2 метра перед детектором по оси Z
 * 4. Установить направление - вдоль оси Z (прямо на детектор)
 * 5. Установить энергию - 100 ГэВ (будет менялась в main)
 *
 * **G4ParticleGun:**
 * Это инструмент GEANT4 для генерации первичных частиц.
 * Позволяет задать:
 * - Тип частицы (G4ParticleDefinition)
 * - Энергию (кинетическая энергия)
 * - Позицию (координаты в пространстве)
 * - Направление (единичный вектор)
 * - Количество частиц за событие
 *
 * **Параметр конструктора:**
 * ```cpp
 * G4ParticleGun(1)  ← 1 частица за событие
 * G4ParticleGun(10) ← 10 частиц за событие (для других экспериментов)
 * ```
 *
 * **Координатная система HERO:**
 * ```
 * Z-axis: вертикальная ось (ось детектора)
 * X,Y-axes: горизонтальные оси
 *
 * Позиция пушки: (0, 0, -2*m) = 2 метра перед детектором
 * Детектор: (0, 0, 0) центр, ±735мм по Z
 * Направление: (0, 0, 1) = прямо на детектор вдоль Z
 *
 *     Пушка
 *    (0,0,-2m)
 *       ↓
 *     -------→ Детектор
 *     Z-axis
 * ```
 *
 * **Начальная энергия:**
 * 100 ГэВ - это начальное значение.
 * Оно будет менялось в main() через SetParticleEnergy().
 *
 * @example
 * ```cpp
 * PrimaryGeneratorAction* primGen = new PrimaryGeneratorAction();
 * // Пушка готова, параметры установлены
 *
 * // Изменить энергию перед запуском
 * primGen->SetParticleEnergy(10.0 * TeV);
 *
 * // Изменить тип частицы
 * primGen->SetParticleType("neutron");
 * ```
 *
 * @note Наследует G4VUserPrimaryGeneratorAction
 * @note G4ParticleGun управляется здесь (удалится в деструкторе)
 *
 * @see GeneratePrimaries() для генерации частиц
 * @see SetParticleEnergy(), SetParticleType() для изменения параметров
 */
PrimaryGeneratorAction::PrimaryGeneratorAction() : G4VUserPrimaryGeneratorAction() {
    // ===== СОЗДАЁМ ПУШКУ =====
    // G4ParticleGun - инструмент GEANT4 для генерации первичных частиц
    // Параметр (1) означает: 1 частица за событие
    fParticleGun = new G4ParticleGun(1);

    // ===== УСТАНАВЛИВАЕМ ПАРАМЕТРЫ ПО УМОЛЧАНИЮ =====

    // **Тип частицы: ПРОТОН**
    // Получаем таблицу частиц GEANT4 и находим протон
    G4ParticleDefinition* particle = G4ParticleTable::GetParticleTable()->FindParticle("proton");
    fParticleGun->SetParticleDefinition(particle);
    
    // **Позиция ПУШКИ: 2 метра ДО детектора**
    // Z = -2м (перед детектором, который находится в центре Z=0)
    // X = 0, Y = 0 (на оси)
    fParticleGun->SetParticlePosition(G4ThreeVector(0, 0, -2*m));
    
    // **НАПРАВЛЕНИЕ: вдоль оси Z (ПРЯМО НА ДЕТЕКТОР)**
    // (0, 0, 1) = единичный вектор вдоль положительной Z
    // Частица будет лететь прямо в детектор
    fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    
    // **ЭНЕРГИЯ: 100 ГэВ (по умолчанию)**
    // Это начальное значение, которое будет менялось в main()
    // через вызов SetParticleEnergy()
    fParticleGun->SetParticleEnergy(100*GeV);
}

/**
 * @brief Деструктор
 *
 * Удаляет G4ParticleGun при завершении программы.
 *
 * @note Вызывается автоматически при выходе из main()
 */
PrimaryGeneratorAction::~PrimaryGeneratorAction() {
    delete fParticleGun;
}

/**
 * @brief Генерировать первичную частицу для события
 *
 * Вызывается в начале каждого события (из GEANT4).
 * Создаёт и добавляет первичную частицу в событие.
 *
 * **Что происходит:**
 * 1. Пушка создаёт частицу с текущими параметрами
 * 2. Частица добавляется в событие
 * 3. GEANT4 начинает отслеживание этой частицы
 * 4. Частица движется через детектор
 * 5. MySteppingAction обрабатывает каждый шаг
 * 6. MyEventAction анализирует результаты
 *
 * **G4ParticleGun::GeneratePrimaryVertex(event):**
 * Это главный метод, который:
 * - Создаёт вершину события (G4PrimaryVertex)
 * - Добавляет в неё первичную частицу (G4PrimaryParticle)
 * - Использует текущие параметры пушки:
 *   ├─ fParticleGun->GetParticleDefinition() - тип
 *   ├─ fParticleGun->GetParticleEnergy() - энергия
 *   ├─ fParticleGun->GetParticlePosition() - позиция
 *   └─ fParticleGun->GetParticleMomentumDirection() - направление
 *
 * **Жизненный цикл события:**
 * ```
 * MyEventAction::BeginOfEventAction()
 *     ↓ Очистить карты нейтронов
 *
 * PrimaryGeneratorAction::GeneratePrimaries()
 *     ↓ Создать первичную частицу
 *
 * [Отслеживание частицы - много шагов]
 *     ↓ Для каждого шага:
 *     ├─ MySteppingAction::UserSteppingAction()
 *     └─ Регистрировать нейтроны
 *
 * MyEventAction::EndOfEventAction()
 *     ↓ Подсчитать нейтроны и записать в CSV
 * ```
 *
 * @param event Указатель на G4Event
 *              Событие, в которое добавляется первичная частица
 *              GEANT4 управляет этим объектом
 *
 * @note Вызывается один раз в начале каждого события
 * @note Используются текущие параметры пушки (могут меняться)
 * @note После этого GEANT4 автоматически отслеживает частицу
 *
 * **Примеры параметров при вызове:**
 * ```cpp
 * // Событие 1: протон 10 ТеВ
 * primGen->SetParticleEnergy(10.0 * TeV);
 * runManager->BeamOn(1);
 * // GeneratePrimaries создаст протон с E=10ТеВ
 *
 * // Событие 2: нейтрон 5 ТеВ
 * primGen->SetParticleType("neutron");
 * primGen->SetParticleEnergy(5.0 * TeV);
 * runManager->BeamOn(1);
 * // GeneratePrimaries создаст нейтрон с E=5ТеВ
 * ```
 *
 * @see PrimaryGeneratorAction() конструктор, инициализация пушки
 * @see SetParticleEnergy() для изменения энергии
 * @see SetParticleType() для изменения типа частицы
 * @see MySteppingAction::UserSteppingAction() для отслеживания
 */
void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event) {
    // Генерируем одну частицу с текущими настройками пушки
    // Пушка использует свои параметры:
    // - SetParticleDefinition() - тип частицы
    // - SetParticleEnergy() - кинетическая энергия
    // - SetParticlePosition() - начальная позиция
    // - SetParticleMomentumDirection() - направление импульса
    fParticleGun->GeneratePrimaryVertex(event);
}