/**
 * @file MySteppingAction.cc
 * @brief Реализация обработчика шагов отслеживания частиц
 *
 * Вызывается для КАЖДОГО ШАГА каждого трека в симуляции.
 * Главная обязанность: регистрировать вторичные нейтроны и их энергию.
 *
 * **Основные функции:**
 * - UserSteppingAction() - обработка каждого шага трека
 * - Reset() - очистка данных перед новым событием
 * - GetDelayedNeutronsMaps() - получить карты нейтронов по задержкам
 * - GetThresholds() - получить пороги временных задержек
 *
 * **Жизненный цикл трека:**
 * ```
 * PrimaryGeneratorAction создаёт первичную частицу (протон)
 *     ↓
 * GEANT4 отслеживает путь протона в детекторе
 *     ↓
 * На каждом шаге:
 *     ├─ MySteppingAction::UserSteppingAction()
 *     │  ├─ Проверить: это нейтрон?
 *     │  ├─ Проверить: находится в Detector?
 *     │  ├─ Получить энергию и время
 *     │  └─ Записать в карту если время >= порога
 *     ↓
 * Трек заканчивается (поглощение, выход из детектора и т.д.)
 *     ↓
 * MyEventAction::EndOfEventAction()
 *     ├─ CountLowEnergyNeutrons()
 *     │  └─ Подсчитать нейтроны из карт
 *     └─ Записать результат в CSV
 * ```
 *
 * **Временные пороги (задержки):**
 * Используются для определения "отложенных" нейтронов.
 * Нейтроны, появившиеся после определённого времени, считаются "задержанными".
 *
 * Примеры временных порогов:
 * ```
 * thresholds[0] = 0.10 µs  ← Нейтроны появились после 0.10 микросекунд
 * thresholds[1] = 0.20 µs  ← Нейтроны появились после 0.20 микросекунд
 * thresholds[2] = 0.50 µs  ← Нейтроны появились после 0.50 микросекунд
 * ```
 *
 * **Карты нейтронов:**
 * ```
 * fDelayedNeutronsMaps[i] = std::map<G4int, G4double>
 *     ├─ Ключ (G4int)    - уникальный ID трека нейтрона
 *     └─ Значение (G4double) - кинетическая энергия нейтрона (в МэВ)
 *
 * Пример карты для первого порога (0.10 µs):
 * {
 *     123: 0.5 MeV,    ← Нейтрон с ID=123, энергия 0.5 МэВ
 *     456: 0.2 MeV,    ← Нейтрон с ID=456, энергия 0.2 МэВ
 *     789: 0.8 MeV     ← Нейтрон с ID=789, энергия 0.8 МэВ
 * }
 * ```
 *
 * @author HERO Collaboration
 * @date 2024
 * @version 1.0
 *
 * @note Вызывается ОЧЕНЬ часто - оптимизация критична!
 * @note Используется для сбора низкоэнергетических нейтронов
 * @see G4UserSteppingAction, MyEventAction
 */

#include "MySteppingAction.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4Neutron.hh"
#include "G4SystemOfUnits.hh"

/**
 * @brief Конструктор с параметрами
 *
 * Инициализирует MySteppingAction с задаными временными порогами.
 *
 * **Процесс инициализации:**
 * 1. Сохраняет переданные пороги времени
 * 2. Создаёт пустые карты нейтронов для каждого порога
 * 3. Карты готовы к заполнению во время симуляции
 *
 * @param thresholds Вектор временных порогов (в единицах GEANT4)
 *                   Обычно это микросекунды (microsecond)
 *                   Пример: {0.10*µs, 0.20*µs, 0.50*µs}
 *                   Каждый порог создаёт отдельную карту нейтронов
 *
 * **Размер структур:**
 * - fThresholds: N порогов
 * - fDelayedNeutronsMaps: N карт (пустых изначально)
 *
 * **Пример использования:**
 * ```cpp
 * std::vector<G4double> delays = {
 *     0.10 * microsecond,
 *     0.20 * microsecond,
 *     0.50 * microsecond
 * };
 * MySteppingAction* stepping = new MySteppingAction(delays);
 * // stepping->fDelayedNeutronsMaps.size() == 3
 * ```
 *
 * @see UserSteppingAction() где используются эти пороги
 * @see Reset() для очистки карт перед каждым событием
 */
MySteppingAction::MySteppingAction(const std::vector<G4double>& thresholds)
: fThresholds(thresholds)
{
    // Создаём пустые карты для каждого временного порога
    // Каждая карта будет содержать нейтроны для своего временного окна
    fDelayedNeutronsMaps.resize(fThresholds.size());
}

/**
 * @brief Деструктор
 *
 * Освобождает ресурсы при удалении объекта.
 * GEANT4 управляет памятью, поэтому обычно пуст.
 *
 * @note Вызывается при завершении симуляции (обычно автоматически)
 */
MySteppingAction::~MySteppingAction() { }

/**
 * @brief Обработка каждого шага трека
 *
 * Вызывается ОДИН РАЗ для КАЖДОГО ШАГА каждого трека в симуляции.
 * Это критичное место для производительности - здесь много операций!
 *
 * **Количество вызовов:**
 * - Для 1000 событий с 100 шагами каждое: 100,000+ вызовов!
 * - В многопоточном режиме: очень много одновременных вызовов
 * - Оптимизация критична!
 *
 * **Алгоритм:**
 * 1. Получить трек из шага
 * 2. Проверить: это нейтрон? (PDG код 2112)
 *    ├─ Если НЕ нейтрон → вернуть (раннее прекращение!)
 * 3. Проверить: находится в Detector?
 *    ├─ Если не в Detector → вернуть (раннее прекращение!)
 * 4. Получить:
 *    ├─ ID трека (уникальный номер)
 *    ├─ Кинетическую энергию после шага
 *    └─ Глобальное время события
 * 5. Для каждого временного порога:
 *    ├─ Если глобальное время >= порог:
 *    │  └─ Записать нейтрон в карту (если ещё не записан)
 *    └─ Иначе: пропустить
 *
 * **Ранние прекращения (оптимизация):**
 * ```cpp
 * if (track->GetDefinition()->GetPDGEncoding() != 2112) return;
 * ↑ 90% треков НЕ нейтроны - выходим сразу!
 *
 * if (!volume || volume->GetName() != "Detector") return;
 * ↑ Нейтроны вне детектора не интересуют - выходим!
 * ```
 *
 * **Проверка дубликатов:**
 * ```cpp
 * if (fDelayedNeutronsMaps[i].find(id) == fDelayedNeutronsMaps[i].end()) {
 *     fDelayedNeutronsMaps[i][id] = kinE;
 * }
 * ```
 * Каждый нейтрон записывается только один раз!
 * Если нейтрон делает несколько шагов в детекторе, записываем только первый раз.
 *
 * @param step Указатель на G4Step (информация о шаге)
 *             Содержит:
 *             - track: информация о частице
 *             - preStepPoint: состояние ДО шага
 *             - postStepPoint: состояние ПОСЛЕ шага
 *
 * **Структура G4Step:**
 * ```
 * G4Step
 * ├─ track
 * │  ├─ GetDefinition() → G4ParticleDefinition
 * │  │  └─ GetPDGEncoding() = 2112 для нейтронов
 * │  └─ GetTrackID() → уникальный ID
 * ├─ preStepPoint
 * │  ├─ GetPosition()
 * │  ├─ GetKineticEnergy()
 * │  └─ GetGlobalTime()
 * └─ postStepPoint
 *    ├─ GetPosition()
 *    ├─ GetKineticEnergy() ← ИСПОЛЬЗУЕМ ЭТО
 *    ├─ GetGlobalTime() ← ИСПОЛЬЗУЕМ ЭТО
 *    └─ GetPhysicalVolume() ← ИСПОЛЬЗУЕМ ЭТО
 * ```
 *
 * **PDG коды частиц:**
 * ```
 * 2112  = нейтрон (neutron)
 * 2212  = протон (proton)
 * 11    = электрон (electron)
 * 22    = фотон (photon)
 * и т.д.
 * ```
 *
 * **Примеры вызовов:**
 * ```cpp
 * // При каждом шаге нейтрона в детекторе:
 * // globalTime=0.05µs, kinE=0.5MeV, ID=123
 * // Пороги: [0.10µs, 0.20µs, 0.50µs]
 *
 * // globalTime < пороги[0], поэтому:
 * // fDelayedNeutronsMaps[0-2] НЕ изменяются
 *
 * // Через некоторое время (globalTime=0.15µs, kinE=0.3MeV, ID=456):
 * // 0.15µs >= 0.10µs → записали в карту[0]
 * // 0.15µs < 0.20µs → НЕ записали в карту[1]
 * // Результат: fDelayedNeutronsMaps[0][456] = 0.3MeV
 * ```
 *
 * @note Вызывается для КАЖДОГО шага - оптимизация критична!
 * @note Только нейтроны в Detector интересуют
 * @note Может быть вызвано из разных потоков одновременно
 * @note Каждый нейтрон записывается только один раз
 *
 * @see Reset() для очистки перед новым событием
 * @see GetDelayedNeutronsMaps() для доступа к результатам
 */
void MySteppingAction::UserSteppingAction(const G4Step* step) {
    // РАНЕЕ ПРЕКРАЩЕНИЕ #1: быстро отбросить не-нейтроны
    // ~90% частиц НЕ нейтроны, поэтому эта проверка экономит время!
    G4Track* track = step->GetTrack();
    
    // Интересуют только нейтроны (PDG код 2112)
    if (track->GetDefinition()->GetPDGEncoding() != 2112) return;
    
    // РАНЕЕ ПРЕКРАЩЕНИЕ #2: только нейтроны в детекторе
    // Нейтроны вне детектора не интересуют
    auto volume = step->GetPostStepPoint()->GetPhysicalVolume();
    if (!volume || volume->GetName() != "Detector") return;

    // ===== РЕГИСТРАЦИЯ НЕЙТРОНА =====
    
    // Получаем информацию о нейтроне
    G4int id = track->GetTrackID();                                    // Уникальный ID
    G4double kinE = step->GetPostStepPoint()->GetKineticEnergy();      // Энергия ПОСЛЕ шага
    G4double globalTime = step->GetPostStepPoint()->GetGlobalTime();   // Время ПОСЛЕ шага
    
    // Начальное распределение больше не записываем (было убрано для упрощения)
    
    // ===== ПРОВЕРКА ВРЕМЕННЫХ ПОРОГОВ =====
    
    // Проверяем каждый временной порог
    for (size_t i = 0; i < fThresholds.size(); ++i) {
        // Если время >= порога, записываем нейтрон в соответствующую карту
        // (нейтрон появился достаточно поздно)
        if (globalTime >= fThresholds[i]) {
            // Записываем только если этот нейтрон ещё не был записан
            // (используем find() для быстрой проверки наличия)
            if (fDelayedNeutronsMaps[i].find(id) == fDelayedNeutronsMaps[i].end()) {
                // ✓ Записали нейтрон в карту
                fDelayedNeutronsMaps[i][id] = kinE;
            }
        }
    }
    
    // **ВАЖНО**: каждый нейтрон записывается только один раз (первый раз в пороге)!
}

/**
 * @brief Очистить все карты нейтронов
 *
 * Удаляет все записанные нейтроны из всех карт.
 * Вызывается в начале каждого события (из MyEventAction::BeginOfEventAction).
 *
 * **Почему это критично:**
 * Данные из разных событий НЕ должны смешиваться!
 * Без Reset() нейтроны из события N-1 останутся в событии N.
 *
 * **Алгоритм:**
 * Для каждой карты в fDelayedNeutronsMaps:
 *     ├─ Очистить карту (удалить все элементы)
 *     └─ Карта готова для нового события
 *
 * **Результат после Reset():**
 * ```
 * Было:
 * fDelayedNeutronsMaps[0] = {123: 0.5, 456: 0.2, 789: 0.8}
 * fDelayedNeutronsMaps[1] = {123: 0.5, 789: 0.8}
 * fDelayedNeutronsMaps[2] = {123: 0.5}
 *
 * После Reset():
 * fDelayedNeutronsMaps[0] = {} (пусто)
 * fDelayedNeutronsMaps[1] = {} (пусто)
 * fDelayedNeutronsMaps[2] = {} (пусто)
 * ```
 *
 * @note Вызывается один раз в начале каждого события
 * @note **КРИТИЧНО**: очищаем ВСЕ карты!
 * @note Если забыть вызвать Reset(), данные смешаются между событиями
 *
 * @see MyEventAction::BeginOfEventAction() откуда вызывается
 * @see UserSteppingAction() где заполняются карты
 */
void MySteppingAction::Reset() {
    // Очищаем все карты перед новым событием
    for (auto& m : fDelayedNeutronsMaps) {
        m.clear();  // Удаляем все элементы из карты
    }
}

/**
 * @brief Получить карту нейтронов начального распределения
 *
 * УСТАРЕВШАЯ функция! Возвращает пустую карту.
 * Была использована для регистрации нейтронов в момент рождения.
 * Теперь убрана для упрощения кода.
 *
 * @return const std::map<G4int, G4double>& Ссылка на пустую карту
 *
 * @deprecated Эта функция больше не используется
 * @note Оставлена для совместимости, но всегда возвращает пустую карту
 *
 * @see GetDelayedNeutronsMaps() для получения актуальных данных
 */
const std::map<G4int, G4double>& MySteppingAction::GetSecondaryNeutrons() const {
    // Возвращаем пустую карту (начальное распределение больше не используется)
    static std::map<G4int, G4double> emptyMap;
    return emptyMap;
}

/**
 * @brief Получить карты нейтронов для всех временных порогов
 *
 * Возвращает вектор карт, где каждая карта содержит нейтроны
 * для соответствующего временного порога.
 *
 * **Структура возвращаемого значения:**
 * ```
 * vector[0] = карта нейтронов для порога[0] (0.10 µs)
 * vector[1] = карта нейтронов для порога[1] (0.20 µs)
 * vector[2] = карта нейтронов для порога[2] (0.50 µs)
 * ...
 * ```
 *
 * **Каждая карта:**
 * ```
 * map<G4int, G4double>
 * ├─ ключ (G4int) = ID трека нейтрона
 * └─ значение (G4double) = энергия нейтрона
 * ```
 *
 * **Примеры:**
 * ```cpp
 * auto& maps = stepping->GetDelayedNeutronsMaps();
 * // maps.size() == 3 (количество порогов)
 * 
 * for (size_t i = 0; i < maps.size(); ++i) {
 *     G4cout << "Порог " << i << ": " << maps[i].size() << " нейтронов" << G4endl;
 * }
 *
 * // Итерировать по нейтронам первого порога
 * for (const auto& entry : maps[0]) {
 *     int trackID = entry.first;
 *     double energy = entry.second;
 *     G4cout << "Нейтрон ID=" << trackID << " Energy=" << energy << G4endl;
 * }
 * ```
 *
 * @return const std::vector<std::map<G4int, G4double>>& 
 *         Ссылка на вектор карт нейтронов
 *
 * @note Используется в MyEventAction::CountLowEnergyNeutrons()
 * @note Каждая карта соответствует одному временному порогу
 * @note Карты пусты в начале события, заполняются во время отслеживания
 *
 * @see UserSteppingAction() где заполняются карты
 * @see Reset() для очистки карт
 * @see GetThresholds() для получения самих порогов
 */
const std::vector<std::map<G4int, G4double>>& MySteppingAction::GetDelayedNeutronsMaps() const {
    return fDelayedNeutronsMaps;
}

/**
 * @brief Получить пороги временных задержек
 *
 * Возвращает вектор временных порогов, переданных в конструктор.
 * Используется для сопоставления карт нейтронов с их временными окнами.
 *
 * **Структура:**
 * ```
 * thresholds[0] = 0.10 * microsecond
 * thresholds[1] = 0.20 * microsecond
 * thresholds[2] = 0.50 * microsecond
 * ...
 * ```
 *
 * **Использование:**
 * ```cpp
 * auto& thresholds = stepping->GetThresholds();
 * auto& maps = stepping->GetDelayedNeutronsMaps();
 *
 * for (size_t i = 0; i < thresholds.size(); ++i) {
 *     G4double delay = thresholds[i] / microsecond;
 *     int count = maps[i].size();
 *     G4cout << "Delay " << delay << " µs: " 
 *            << count << " neutrons" << G4endl;
 * }
 * ```
 *
 * @return const std::vector<G4double>& Ссылка на вектор порогов
 *
 * @note Размер совпадает с размером fDelayedNeutronsMaps
 * @note Пороги установлены в конструкторе и не меняются
 * @note Используется для форматирования вывода и логирования
 *
 * @see GetDelayedNeutronsMaps() для соответствующих карт
 * @see MySteppingAction() конструктор, где устанавливаются пороги
 */
const std::vector<G4double>& MySteppingAction::GetThresholds() const {
    return fThresholds;
}