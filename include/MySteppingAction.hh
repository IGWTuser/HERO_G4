/**
 * @file MySteppingAction.hh
 * @brief Анализ каждого шага траектории частицы
 *
 * Вызывается для каждого шага при отслеживании трека.
 * Основное назначение: регистрация вторичных нейтронов в разных временных окнах.
 *
 * Основные функции:
 * - UserSteppingAction() - обработка каждого шага
 * - Регистрация рождения вторичных нейтронов
 * - Подсчёт нейтронов для разных временных задержек (thresholds)
 * - Reset() - очистка данных перед новым событием
 *
 * **ВНИМАНИЕ**: Это может быть вызвано миллионы раз в секунду!
 * Оптимизация КРИТИЧНА для производительности.
 *
 * Структура данных для временных окон:
 * ```
 * Задержка 1 (0.10 µs)  → карта нейтронов до 0.10 µs
 * Задержка 2 (0.20 µs)  → карта нейтронов до 0.20 µs
 * Задержка 3 (0.50 µs)  → карта нейтронов до 0.50 µs
 * ```
 *
 * @date 2024
 * @version 1.0
 *
 * @note В многопоточном режиме каждый поток имеет свою копию этого класса
 * @note Данные используются в MyEventAction для подсчёта низкоэнергетических нейтронов
 * @see G4UserSteppingAction, MyEventAction
 */

#ifndef MySteppingAction_h
#define MySteppingAction_h 1

#include "G4UserSteppingAction.hh"
#include "globals.hh"
#include <map>
#include <vector>

/**
 * @class MySteppingAction
 * @brief Обработка каждого шага отслеживания частицы
 *
 * Вызывается после каждого шага при отслеживании траектории частицы.
 * Шаг ограничен либо:
 * - Расстоянием свободного пробега (взаимодействие - рождение вторичных)
 * - Границей материала
 * - Максимальной длиной (установленной пользователем)
 *
 * В нашем проекте отвечает за:
 * 1. **Регистрация вторичных нейтронов**
 *    - Обнаружение рождения нейтронов при взаимодействии
 *    - Запись энергии нейтрона
 *    - Запись времени рождения нейтрона
 *
 * 2. **Подсчёт по временным окнам**
 *    - Для каждого порога (например, 0.10, 0.20, 0.50 µs)
 *    - Подсчитать нейтроны, рождённые ДО этого времени
 *    - Сохранить в отдельную карту для каждого порога
 *
 * 3. **Оптимизация производительности**
 *    - Минимум операций в UserSteppingAction (критический код)
 *    - Использование быстрых структур данных (std::map)
 *    - Избегание выделения памяти во время отслеживания
 *
 * **Структура данных:**
 * ```
 * fThresholds = [0.10 µs, 0.20 µs, 0.50 µs]
 *
 * fDelayedNeutronsMaps[0] = {
 *     TrackID 5 → Energy 0.5 eV  (нейтрон рожден до 0.10 µs)
 *     TrackID 7 → Energy 1.2 eV  (нейтрон рожден до 0.10 µs)
 * }
 *
 * fDelayedNeutronsMaps[1] = {
 *     TrackID 5 → Energy 0.5 eV  (нейтрон рожден до 0.20 µs)
 *     TrackID 7 → Energy 1.2 eV  (нейтрон рожден до 0.20 µs)
 *     TrackID 12 → Energy 0.8 eV (нейтрон рожден до 0.20 µs)
 * }
 * ```
 *
 * @note **ПРОИЗВОДИТЕЛЬНОСТЬ**: Этот класс вызывается ОЧЕНЬ часто!
 *       Весь код должен быть максимально оптимизирован.
 *
 * @see G4UserSteppingAction для базового класса
 * @see MyEventAction для использования собранных данных
 */
class MySteppingAction : public G4UserSteppingAction {
public:
    /**
     * @brief Конструктор с параметрами
     *
     * Инициализирует SteppingAction с временными порогами для подсчёта нейтронов.
     *
     * @param thresholds Вектор временных порогов (в единицах GEANT4)
     *                   Каждый элемент соответствует одному временному окну
     *                   Должны быть отсортированы по возрастанию
     *                   Пример: {0.10 * microsecond, 0.20 * microsecond, 0.50 * microsecond}
     *
     * Процесс инициализации:
     * 1. Сохранить thresholds в fThresholds
     * 2. Создать fDelayedNeutronsMaps.size() = thresholds.size()
     * 3. Каждый элемент fDelayedNeutronsMaps[i] - пустая карта (TrackID → Energy)
     *
     * @example
     * ```cpp
     * std::vector<G4double> delays = {0.10 * microsecond, 0.20 * microsecond, 0.50 * microsecond};
     * MySteppingAction* stepping = new MySteppingAction(delays);
     * ```
     *
     * @note thresholds должны быть отсортированы! (возрастающий порядок)
     * @see Reset() для очистки данных перед новым событием
     */
    MySteppingAction(const std::vector<G4double>& thresholds);
    
    /**
     * @brief Деструктор
     *
     * Освобождает память, выделенную для карт нейтронов.
     * std::map и std::vector очищаются автоматически.
     */
    virtual ~MySteppingAction();

    /**
     * @brief Обработить один шаг траектории частицы
     *
     * Вызывается после каждого шага отслеживания любой частицы.
     * Проверяет, родился ли в этом шаге новый нейтрон,
     * и если да, регистрирует его энергию и время для разных порогов.
     *
     * **Алгоритм:**
     * 1. Проверить, есть ли вторичные треки (вторичные частицы) на этом шаге
     * 2. Для каждого вторичного трека:
     *    a. Проверить, является ли он нейтроном
     *    b. Получить его кинетическую энергию
     *    c. Получить время рождения
     *    d. Для каждого порога времени:
     *       - Если время < порог: добавить в соответствующую карту
     * 3. Закончить (не выполнять сложные операции!)
     *
     * **Структура G4Step:**
     * ```
     * step->GetPreStepPoint()  - состояние ДО взаимодействия
     * step->GetPostStepPoint() - состояние ПОСЛЕ взаимодействия
     * step->GetTrack()         - информация о текущем треке
     * step->GetSecondaryInCurrentStep() - список вторичных частиц, рождённых на этом шаге
     * ```
     *
     * **Получение вторичных частиц:**
     * ```cpp
     * G4TrackVector* secondaries = step->GetSecondaryInCurrentStep();
     * for (size_t i = 0; i < secondaries->size(); ++i) {
     *     G4Track* secondary = (*secondaries)[i];
     *     G4String particleName = secondary->GetDefinition()->GetParticleName();
     *     if (particleName == "neutron") {
     *         G4double energy = secondary->GetKineticEnergy();
     *         G4double time = secondary->GetGlobalTime();
     *         // обработать нейтрон
     *     }
     * }
     * ```
     *
     * @param step Указатель на G4Step с информацией о шаге
     *             Содержит:
     *             - Первичный трек (исходная частица)
     *             - Вторичные треки (рождённые частицы)
     *             - Информацию о взаимодействии
     *             - Времени и энергии
     *
     * @note **КРИТИЧНО**: НЕ ВЫПОЛНЯЙТЕ СЛОЖНЫЕ ОПЕРАЦИИ!
     *       Это замедлит программу в 10+ раз
     * @note Вызывается МИЛЛИОНЫ раз за время симуляции
     * @note **ОПТИМИЗАЦИЯ**: Используйте быстрые контейнеры (std::map, std::vector)
     *
     * @see GetSecondaryNeutrons() для доступа к собранным данным
     * @see GetDelayedNeutronsMaps() для доступа к картам по задержкам
     * @see MyEventAction::CountLowEnergyNeutrons() для использования данных
     */
    virtual void UserSteppingAction(const G4Step* step);
    
    /**
     * @brief Очистить данные перед новым событием
     *
     * Обнуляет все карты нейтронов и готовит к новому событию.
     * Должна быть вызвана в MyEventAction::BeginOfEventAction().
     *
     * Операции:
     * - fSecondaryNeutronsMap.clear()
     * - Для каждого элемента в fDelayedNeutronsMaps: clear()
     *
     * @example
     * ```cpp
     * // В MyEventAction::BeginOfEventAction()
     * void MyEventAction::BeginOfEventAction(const G4Event* event) {
     *     fSteppingAction->Reset();  // очистить данные предыдущего события
     *     fNeutronCount = 0;
     * }
     * ```
     *
     * @see UserSteppingAction()
     * @note Должна быть вызвана ровно один раз в начале каждого события
     * @note **ВАЖНО**: без Reset() данные из разных событий смешаются!
     *
     * @see MyEventAction::BeginOfEventAction()
     */
    void Reset();

    /**
     * @brief Получить карту начальных энергий нейтронов
     *
     * Возвращает карту TrackID → Energy для всех вторичных нейтронов,
     * рождённых в этом событии (без учёта временных задержек).
     *
     * **ВНИМАНИЕ**: Эта карта больше не используется в новой версии кода.
     * Вместо неё используйте GetDelayedNeutronsMaps().
     *
     * @return const std::map<G4int, G4double>&
     *         Карта, где:
     *         - Ключ (G4int) = TrackID нейтрона (уникальный ID в этом событии)
     *         - Значение (G4double) = кинетическая энергия (в МэВ)
     *
     * @example
     * ```cpp
     * const auto& neutrons = steppingAction->GetSecondaryNeutrons();
     * for (const auto& pair : neutrons) {
     *     G4int trackID = pair.first;
     *     G4double energy = pair.second;
     *     G4cout << "Track " << trackID << ": " << energy / eV << " eV\n";
     * }
     * ```
     *
     * @deprecated Используйте GetDelayedNeutronsMaps() вместо этого метода
     * @see GetDelayedNeutronsMaps()
     */
    const std::map<G4int, G4double>& GetSecondaryNeutrons() const;

    /**
     * @brief Получить все карты нейтронов для разных временных задержек
     *
     * Возвращает вектор карт, где каждый элемент соответствует одной временной задержке.
     * Используется для подсчёта низкоэнергетических нейтронов в разных временных окнах.
     *
     * **Структура результата:**
     * ```
     * fDelayedNeutronsMaps[0] = {TrackID → Energy} - нейтроны до thresholds[0]
     * fDelayedNeutronsMaps[1] = {TrackID → Energy} - нейтроны до thresholds[1]
     * ...
     * fDelayedNeutronsMaps[n] = {TrackID → Energy} - нейтроны до thresholds[n]
     * ```
     *
     * @return const std::vector<std::map<G4int, G4double>>&
     *         Вектор карт (один элемент на каждый порог)
     *
     * @example
     * ```cpp
     * const auto& maps = steppingAction->GetDelayedNeutronsMaps();
     * const auto& thresholds = steppingAction->GetThresholds();
     *
     * for (size_t i = 0; i < maps.size(); ++i) {
     *     G4int count = maps[i].size();
     *     G4double delay = thresholds[i];
     *     G4cout << "Neutrons up to " << delay/microsecond << " µs: " << count << "\n";
     * }
     * ```
     *
     * @see GetThresholds() для получения значений временных порогов
     * @see MyEventAction::CountLowEnergyNeutrons() для использования
     */
    const std::vector<std::map<G4int, G4double>>& GetDelayedNeutronsMaps() const;
    
    /**
     * @brief Получить вектор временных порогов
     *
     * Возвращает вектор временных задержек, для которых подсчитываются нейтроны.
     * Соответствует порядку карт в GetDelayedNeutronsMaps().
     *
     * @return const std::vector<G4double>&
     *         Вектор временных порогов (в единицах GEANT4, обычно микросекунды)
     *         Пример: {0.10 µs, 0.20 µs, 0.50 µs}
     *
     * @example
     * ```cpp
     * const auto& thresholds = steppingAction->GetThresholds();
     * for (size_t i = 0; i < thresholds.size(); ++i) {
     *     G4cout << "Delay " << i << ": " << thresholds[i]/microsecond << " µs\n";
     * }
     * ```
     *
     * @see GetDelayedNeutronsMaps()
     */
    const std::vector<G4double>& GetThresholds() const;

private:
    /// @brief Карта начальных энергий вторичных нейтронов
    ///
    /// **ВНИМАНИЕ**: Эта переменная больше не используется в текущей версии.
    /// Вместо неё используйте fDelayedNeutronsMaps.
    ///
    /// Структура (исторически):
    /// - Ключ: TrackID нейтрона (уникальный ID в этом событии)
    /// - Значение: кинетическая энергия (в МэВ)
    ///
    /// Сохраняется для обратной совместимости.
    /// В новом коде используйте fDelayedNeutronsMaps вместо этого.
    std::map<G4int, G4double> fSecondaryNeutronsMap;

    /// @brief Вектор временных порогов для подсчёта нейтронов
    ///
    /// Каждый элемент - это время, до которого подсчитываются нейтроны.
    /// Например: {0.10 µs, 0.20 µs, 0.50 µs}
    ///
    /// Инициализируется в конструкторе.
    /// Должны быть отсортированы по возрастанию!
    ///
    /// Используется в UserSteppingAction() для определения,
    /// в какие карты добавлять рождённый нейтрон.
    std::vector<G4double> fThresholds;
    
    /// @brief Вектор карт нейтронов для разных временных порогов
    ///
    /// Размер = fThresholds.size()
    /// Каждый элемент [i] содержит карту нейтронов, рождённых до fThresholds[i]
    ///
    /// Структура каждой карты:
    /// - Ключ (G4int): TrackID нейтрона (уникальный в этом событии)
    /// - Значение (G4double): кинетическая энергия нейтрона (в МэВ)
    ///
    /// Пример:
    /// ```
    /// fDelayedNeutronsMaps[0] - все нейтроны до 0.10 µs
    /// fDelayedNeutronsMaps[1] - все нейтроны до 0.20 µs (включает из [0])
    /// fDelayedNeutronsMaps[2] - все нейтроны до 0.50 µs (включает из [1])
    /// ```
    ///
    /// Очищается в Reset() перед каждым новым событием.
    /// Заполняется в UserSteppingAction() при регистрации нейтронов.
    /// Читается в MyEventAction::CountLowEnergyNeutrons() для анализа.
    std::vector<std::map<G4int, G4double>> fDelayedNeutronsMaps;
};

#endif