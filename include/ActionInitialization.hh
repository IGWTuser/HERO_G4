#ifndef ActionInitialization_h
#define ActionInitialization_h 1

/**
 * @file ActionInitialization.hh
 * @brief Инициализация всех Action классов для многопоточного режима GEANT4
 *
 * Содержит класс, который создает и регистрирует Action классы:
 * - PrimaryGeneratorAction (источник частиц с параметрами)
 * - MyRunAction (управление сериями)
 * - MyEventAction (сбор данных события)
 * - MySteppingAction (анализ каждого шага)
 *
 * Этот класс вызывается G4RunManager при инициализации и поддерживает
 * многопоточный режим (G4MTRunManager), создавая отдельные экземпляры
 * Action-классов для каждого рабочего потока.
 *
 * @date 2024
 * @version 1.0
 *
 * @note В многопоточном режиме:
 *       - BuildForMaster() вызывается один раз в главном потоке
 *       - Build() вызывается один раз для каждого рабочего потока
 * @see G4VUserActionInitialization, CSVWriter
 */

#include "G4VUserActionInitialization.hh"
#include "globals.hh"
#include <vector>

// Форвард деклерация (экономит время компиляции)
class CSVWriter;

/**
 * @class ActionInitialization
 * @brief Инициализатор всех пользовательских Action классов
 *
 * Наследуется из G4VUserActionInitialization. Требует реализации двух методов:
 * - BuildForMaster() - для главного потока (многопоточность)
 * - Build() - для каждого рабочего потока
 *
 * Отвечает за создание экземпляров:
 * - PrimaryGeneratorAction (передаёт параметры частицы и энергию)
 * - MyRunAction (управление сериями)
 * - MyEventAction (сбор данных события)
 * - MySteppingAction (анализ каждого шага траектории)
 *
 * Параметры передаются через конструктор и используются всеми потоками:
 * - delays: временные диапазоны для анализа энергии
 * - csvWriter: единый экземпляр для записи результатов (синглтон)
 * - particleName: тип частицы (например, "proton", "neutron")
 * - energy: энергия первичной частицы (в ТэВ)
 */
class ActionInitialization : public G4VUserActionInitialization {
public:
    /**
     * @brief Конструктор инициализатора Action классов
     *
     * Сохраняет параметры, которые будут использоваться всеми worker-потоками
     * при создании PrimaryGeneratorAction и других классов.
     *
     * @param delays Вектор временных диапазонов (в микросекундах или других единицах)
     *               для подсчёта энергии в разных временных окнах
     *               Пример: {0.10, 0.20, 0.50} µs
     *
     * @param csvWriter Указатель на синглтон CSVWriter для записи результатов
     *                  Один экземпляр используется для всех потоков
     *                  @warning csvWriter НЕ ДОЛЖЕН быть nullptr
     *
     * @param particleName Название типа частицы (например, "proton", "neutron")
     *                     Определяет, какая частица будет генерироваться
     *                     Передаётся в PrimaryGeneratorAction
     *
     * @param energy Кинетическая энергия первичной частицы (в ТэВ)
     *               Пример: 10.0 ТэВ для адронного каскада
     *
     * @see CSVWriter::GetInstance(), PrimaryGeneratorAction
     */
    ActionInitialization(const std::vector<G4double>& delays, 
                        CSVWriter* csvWriter,
                        const G4String& particleName,
                        G4double energy);
    
    /// @brief Деструктор (НЕ удаляет fCSVWriter - он синглтон!)
    virtual ~ActionInitialization();

    /**
     * @brief Создать Action классы для главного потока
     *
     * Вызывается один раз при использовании многопоточного режима (G4MTRunManager).
     * В нашем проекте оставляется пустым (переопределение требует интерфейс),
     * так как все Action-объекты создаются в Build() для каждого потока.
     *
     * Альтернативный вариант: здесь можно создать отдельный RunAction
     * для сбора результатов из всех потоков.
     *
     * @see Build()
     * @note Этот метод НЕ вызывается в однопоточном режиме (обычный G4RunManager)
     */
    virtual void BuildForMaster() const override;
    
    /**
     * @brief Создать Action классы для рабочего потока
     *
     * Вызывается один раз для каждого рабочего потока (в G4MTRunManager)
     * или один раз для всей программы (в обычном G4RunManager).
     *
     * Создаёт экземпляры:
     * - PrimaryGeneratorAction с параметрами (частица, энергия)
     * - MyEventAction для сбора данных события
     * - MySteppingAction для анализа каждого шага
     * - MyRunAction для управления сериями
     *
     * Эти объекты будут удалены G4RunManager автоматически.
     *
     * Порядок важен для понимания потока выполнения:
     * 1. PrimaryGeneratorAction - генерирует первичные частицы
     * 2. MyEventAction - подготовка к событию (инициализация)
     * 3. MySteppingAction - анализ каждого шага
     * 4. MyRunAction - финализация series
     *
     * @see BuildForMaster()
     * @see PrimaryGeneratorAction, MyEventAction, MySteppingAction
     */
    virtual void Build() const override;

private:
    /// @brief Вектор временных диапазонов (задержек) для подсчёта энергии
    ///
    /// Используется для анализа энергии в разных временных окнах.
    /// Пример: [0.10 µs, 0.20 µs, 0.50 µs] - энергия за первые 0.1, 0.2, 0.5 микросекунд
    /// Передаётся в PrimaryGeneratorAction и используется для фильтрации результатов
    std::vector<G4double> fDelays;
    
    /// @brief Указатель на синглтон CSVWriter для записи результатов
    ///
    /// Один экземпляр используется для записи из всех потоков.
    /// НЕ удаляется в деструкторе - управляется главной программой.
    /// Используется для сохранения данных событий в CSV файл.
    CSVWriter* fCSVWriter;
    
    /// @brief Название типа первичной частицы (например, "proton", "neutron")
    ///
    /// Передаётся в PrimaryGeneratorAction::GeneratePrimaries()
    /// для определения, какая частица будет генерироваться.
    /// Может быть изменено через параметры main программы.
    G4String fParticleName;
    
    /// @brief Кинетическая энергия первичной частицы (в ТэВ)
    ///
    /// Передаётся в PrimaryGeneratorAction для установки энергии.
    /// Пример: 10.0 ГэВ для адронных взаимодействий
    /// Может быть изменено через параметры main программы.
    G4double fEnergy;
};

#endif
