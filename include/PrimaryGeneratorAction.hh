/**
 * @file PrimaryGeneratorAction.hh
 * @brief Генерация первичных частиц для симуляции
 *
 * Отвечает за создание и инициализацию первичной частицы,
 * которая затем отслеживается в детекторе.
 * Вызывается один раз для каждого события.
 *
 * Основные функции:
 * - GeneratePrimaries() - создание первичной частицы для события
 * - GetParticleGun() - доступ к объекту пушки для изменения параметров
 * - Установка энергии, импульса, направления, типа частицы
 *
 * **Связь с иерархией:**
 * ```
 * Simulation (вся программа)
 * ├── Run (серия событий)
 * │   └── Event 1
 * │       ├── PrimaryGeneratorAction::GeneratePrimaries() ← ЗДЕСЬ
 * │       │   Создаёт первичную частицу
 * │       ├── MySteppingAction::UserSteppingAction() (для каждого шага)
 * │       │   Регистрирует вторичные частицы
 * │       └── MyEventAction::EndOfEventAction()
 * │           Анализирует событие
 * ```
 *
 * Пример использования:
 * ```cpp
 * // В main функции:
 * PrimaryGeneratorAction* generator = new PrimaryGeneratorAction();
 * G4ParticleGun* gun = generator->GetParticleGun();
 *
 * // Установить тип частицы
 * G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
 * G4ParticleDefinition* particle = particleTable->FindParticle("proton");
 * gun->SetParticleDefinition(particle);
 *
 * // Установить энергию
 * gun->SetParticleEnergy(10.0 * GeV);
 *
 * // Установить позицию и направление
 * gun->SetParticlePosition(G4ThreeVector(0., 0., 0.));
 * gun->SetParticleMomentumDirection(G4ThreeVector(0., 0., 1.));
 *
 * // Теперь при BeamOn(1000) будет создано 1000 протонов 10 ГэВ
 * ```
 *
 * @date 2024
 * @version 1.0
 *
 * @note G4ParticleGun - встроенный класс GEANT4 для создания частиц
 * @see G4VUserPrimaryGeneratorAction, G4ParticleGun
 */

#ifndef PrimaryGeneratorAction_h
#define PrimaryGeneratorAction_h 1

#include "G4VUserPrimaryGeneratorAction.hh"

// Форвард деклерация
class G4ParticleGun;

/**
 * @class PrimaryGeneratorAction
 * @brief Генератор первичных частиц для каждого события
 *
 * Наследуется из G4VUserPrimaryGeneratorAction.
 * Главный метод: GeneratePrimaries(G4Event* event)
 *
 * **Отвечает за:**
 * 1. **Инициализацию пушки** (в конструкторе)
 *    - Создание объекта G4ParticleGun
 *    - Установка дефолтных параметров
 *    - (Дополнительная параметризация - в main функции)
 *
 * 2. **Генерацию частиц** (в GeneratePrimaries)
 *    - Создание одной первичной частицы
 *    - Вызов gun->GeneratePrimaryVertex(event)
 *
 * 3. **Предоставление доступа к пушке** (GetParticleGun)
 *    - Чтобы main мог менять параметры
 *
 * **Жизненный цикл события:**
 * ```
 * MyRunAction::BeginOfRunAction()
 *     ↓
 * [для каждого события в Run-е]
 *     ↓
 *     PrimaryGeneratorAction::GeneratePrimaries()  ← ЗДЕСЬ
 *     ├─ Используются текущие параметры gun
 *     ├─ Создаётся одна первичная частица
 *     └─ gun->GeneratePrimaryVertex(event) создаёт вершину события
 *     ↓
 *     [отслеживание первичной частицы]
 *     ↓
 *     MySteppingAction::UserSteppingAction() (для каждого шага)
 *     ├─ Регистрация вторичных частиц
 *     └─ Заполнение карт нейтронов
 *     ↓
 *     MyEventAction::EndOfEventAction()
 *     └─ Анализ и запись в CSV
 *     ↓
 * MyRunAction::EndOfRunAction()
 * ```
 *
 * **Параметры G4ParticleGun, которые можно менять:**
 * - SetParticleDefinition(G4ParticleDefinition*) - тип частицы
 * - SetParticleEnergy(G4double) - кинетическая энергия
 * - SetParticleMomentum(G4double) - импульс (альтернатива энергии)
 * - SetParticlePosition(G4ThreeVector) - позиция рождения
 * - SetParticleMomentumDirection(G4ThreeVector) - направление вылета
 * - SetNumberOfParticles(G4int) - количество частиц за раз
 *
 * @see G4VUserPrimaryGeneratorAction для базового класса
 * @see G4ParticleGun для объекта пушки
 * @see MyRunAction для установки параметров Run-а
 */
class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
public:
    /**
     * @brief Конструктор по умолчанию
     *
     * Инициализирует PrimaryGeneratorAction и создаёт объект G4ParticleGun.
     *
     * Операции:
     * 1. Создать новый G4ParticleGun (по умолчанию с одной частицей)
     * 2. Установить дефолтные параметры (если требуется)
     *
     * После конструктора обычно вызывают GetParticleGun() для установки параметров.
     *
     * @example
     * ```cpp
     * PrimaryGeneratorAction* generator = new PrimaryGeneratorAction();
     *
     * // Получить пушку и установить параметры
     * G4ParticleGun* gun = generator->GetParticleGun();
     * G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
     * G4ParticleDefinition* proton = particleTable->FindParticle("proton");
     * gun->SetParticleDefinition(proton);
     * gun->SetParticleEnergy(10.0 * GeV);
     * ```
     *
     * @see GetParticleGun()
     * @note G4ParticleGun создаётся с одной частицей по умолчанию
     */
    PrimaryGeneratorAction();
    
    /**
     * @brief Деструктор
     *
     * Освобождает память, выделенную для G4ParticleGun.
     * Вызывается при удалении объекта (обычно в конце программы).
     */
    virtual ~PrimaryGeneratorAction();

    /**
     * @brief Генерация первичной частицы для события
     *
     * Вызывается один раз в начале каждого события (ДО отслеживания).
     * Создаёт первичную частицу с текущими параметрами пушки.
     *
     * **Алгоритм:**
     * 1. Получить текущие параметры из gun (частица, энергия, направление)
     * 2. Вызвать gun->GeneratePrimaryVertex(event)
     *    - Создаёт вершину события с первичной частицей
     *    - Добавляет первичный трек в событие
     * 3. Закончить (дальше идёт отслеживание в MySteppingAction)
     *
     * **Типичная реализация:**
     * ```cpp
     * void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event) {
     *     // Использовать текущие параметры gun и создать вершину
     *     fParticleGun->GeneratePrimaryVertex(event);
     * }
     * ```
     *
     * **Если нужна параметризация:**
     * ```cpp
     * void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event) {
     *     // Случайное направление
     *     G4double theta = CLHEP::RandFlat::shoot() * 2.0 * M_PI;
     *     G4double phi = CLHEP::RandFlat::shoot() * M_PI;
     *     G4ThreeVector direction(sin(phi)*cos(theta), sin(phi)*sin(theta), cos(phi));
     *     fParticleGun->SetParticleMomentumDirection(direction);
     *
     *     // Случайная энергия (в диапазоне)
     *     G4double energy = 5.0 * GeV + CLHEP::RandFlat::shoot() * 5.0 * GeV;
     *     fParticleGun->SetParticleEnergy(energy);
     *
     *     // Создать вершину
     *     fParticleGun->GeneratePrimaryVertex(event);
     * }
     * ```
     *
     * @param event Указатель на G4Event (событие, в которое добавляется первичная вершина)
     *              После этого вызова событие содержит одну первичную вершину
     *              с одной первичной частицей
     *
     * @note Вызывается один раз в начале каждого события
     * @note Текущие параметры gun устанавливаются в main функции
     *       перед вызовом runManager->BeamOn(nEvents)
     * @note После этого вызова происходит отслеживание частицы
     *       (MySteppingAction вызывается для каждого шага)
     *
     * @see GetParticleGun() для доступа к параметрам
     * @see G4ParticleGun::GeneratePrimaryVertex()
     * @see MySteppingAction::UserSteppingAction() - следующий этап
     */
    virtual void GeneratePrimaries(G4Event* event);
    
    /**
     * @brief Получить указатель на объект пушки
     *
     * Возвращает указатель на G4ParticleGun для изменения параметров.
     * Используется в main функции для установки типа частицы, энергии и т.д.
     *
     * **Параметры, которые можно менять через gun:**
     * - Тип частицы: SetParticleDefinition(particle)
     * - Энергия: SetParticleEnergy(energy)
     * - Импульс: SetParticleMomentum(momentum)
     * - Позиция: SetParticlePosition(position)
     * - Направление: SetParticleMomentumDirection(direction)
     * - Количество частиц за раз: SetNumberOfParticles(n)
     *
     * @return G4ParticleGun* Указатель на объект пушки (создан в конструкторе)
     *         НЕ удаляйте этот указатель - он управляется PrimaryGeneratorAction!
     *
     * @example
     * ```cpp
     * PrimaryGeneratorAction* generator = new PrimaryGeneratorAction();
     * G4ParticleGun* gun = generator->GetParticleGun();
     *
     * // Установить тип частицы
     * G4ParticleTable* table = G4ParticleTable::GetParticleTable();
     * G4ParticleDefinition* proton = table->FindParticle("proton");
     * gun->SetParticleDefinition(proton);
     *
     * // Установить энергию
     * gun->SetParticleEnergy(10.0 * GeV);
     *
     * // Установить позицию и направление
     * gun->SetParticlePosition(G4ThreeVector(0., 0., 0.));
     * gun->SetParticleMomentumDirection(G4ThreeVector(0., 0., 1.));
     *
     * // Теперь при BeamOn(1000) будет 1000 событий
     * // Каждое событие создаст один протон 10 ГэВ
     * ```
     *
     * @note Возвращает тот же указатель, что создан в конструкторе
     * @note НЕ удаляйте указатель - деструктор сделает это автоматически
     * @note Параметры остаются неизменными между событиями (в одном Run-е)
     *
     * @see SetParticleDefinition() для смены типа частицы
     * @see SetParticleEnergy() для смены энергии
     * @see GeneratePrimaries() - использует текущие параметры gun
     */
    G4ParticleGun* GetParticleGun() const { return fParticleGun; }
    
private:
    /// @brief Указатель на объект пушки для создания первичных частиц
    ///
    /// G4ParticleGun - встроенный класс GEANT4 для генерации частиц.
    /// Содержит все параметры первичной частицы:
    /// - Тип (G4ParticleDefinition)
    /// - Энергия (G4double)
    /// - Позиция (G4ThreeVector)
    /// - Направление (G4ThreeVector)
    ///
    /// Создаётся в конструкторе, удаляется в деструкторе.
    /// Используется в GeneratePrimaries() для создания первичной вершины.
    /// Параметры устанавливаются в main функции через GetParticleGun().
    ///
    /// **Типичный жизненный цикл:**
    /// 1. Конструктор: создать new G4ParticleGun()
    /// 2. main: получить через GetParticleGun() и установить параметры
    /// 3. GeneratePrimaries: использовать текущие параметры
    /// 4. Деструктор: delete fParticleGun
    ///
    /// @note НЕ удаляйте этот указатель вручную!
    /// @see GeneratePrimaries() для использования
    /// @see GetParticleGun() для доступа
    G4ParticleGun* fParticleGun;
};

#endif