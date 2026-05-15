# DesertGame — Техническая документация

> Unreal Engine 5.6 · Third-Person Action · Desert Setting

---

## Содержание

1. [Обзор проекта](#обзор-проекта)
2. [Структура исходного кода](#структура-исходного-кода)
3. [Система игрока](#система-игрока)
4. [Система траверсала](#система-траверсала)
5. [Боевая система](#боевая-система)
6. [Система врагов (AI)](#система-врагов-ai)
7. [Конфигурация сборки](#конфигурация-сборки)
8. [Контент-ассеты](#контент-ассеты)
9. [Иерархия классов](#иерархия-классов)

---

## Обзор проекта

| Параметр | Значение |
|---|---|
| Движок | Unreal Engine 5.6 |
| Жанр | Third-Person Action |
| Язык | C++ |
| Система ввода | Enhanced Input System |
| AI | Behavior Tree + Blackboard + AI Perception |
| Ключевые плагины | MotionWarping, ModelingToolsEditorMode |

---

## Структура исходного кода

```
Source/DesertGame/
├── DesertGame.Build.cs               — зависимости модуля
├── Public/
│   ├── ProtagonistCharacter/         — игровой персонаж
│   ├── Traversal/                    — система преодоления препятствий
│   ├── Combat/                       — боевая система
│   └── Enemy/                        — AI врагов
└── Private/
    ├── ProtagonistCharacter/
    ├── Traversal/
    ├── Combat/
    └── Enemy/
```

---

## Система игрока

### `AProtagonistCharacter` — главный персонаж

**Файлы:** [ProtagonistCharacter.h](Source/DesertGame/Public/ProtagonistCharacter/ProtagonistCharacter.h) / [.cpp](Source/DesertGame/Private/ProtagonistCharacter/ProtagonistCharacter.cpp)

Центральный класс игрока. Агрегирует все компоненты и обрабатывает Enhanced Input.

**Компоненты:**
| Компонент | Тип | Назначение |
|---|---|---|
| CameraBoom | USpringArmComponent | Рычаг камеры (300 cm, смещение вправо 50 cm) |
| FollowCamera | UCameraComponent | Камера третьего лица |
| TraversalComponent | UTraversalComponent | Преодоление препятствий |
| MotionWarpingComponent | UMotionWarpingComponent | Варпинг корневого движения |
| CombatComponent | UCombatComponent | Комбо, блок, уклонение |
| StimuliSourceComponent | UAIPerceptionStimuliSourceComponent | Делает персонажа видимым для AI |

**Состояния движения:**

| Enum | Значения |
|---|---|
| `EGaitState` | Walk (200 cm/s), Run (500 cm/s), Sprint (800 cm/s) |
| `EMovementState` | Grounded, Falling, Jumping |
| `EStanceState` | Standing, Crouching (множитель ×0.5) |

**Input Actions (Enhanced Input):**
`Move`, `Look`, `Jump`, `Sprint`, `Crouch`, `Attack`, `Block`, `Dodge`

**Ключевые методы:**
- `StartJump()` — сначала пробует траверсал, затем обычный прыжок
- `OnAttackInput()` / `OnBlockStart/Stop()` / `OnDodgeInput()` — передают ввод в CombatComponent
- `UpdateMovementState()` / `UpdateGaitState()` / `UpdateMaxSpeed()` — синхронизируют скорость с текущим состоянием

---

### `UProtagonistAnimInstance` — Animation Instance

**Файлы:** [ProtagonistAnimInstance.h](Source/DesertGame/Public/ProtagonistCharacter/ProtagonistAnimInstance.h) / [.cpp](Source/DesertGame/Private/ProtagonistCharacter/ProtagonistAnimInstance.cpp)

Мост между Animation Blueprint и состоянием персонажа. Обновляется каждый кадр через `NativeUpdateAnimation()`.

Предоставляет Animation BP:
- `Speed`, `Direction`, `GroundSpeed`, `bShouldMove`
- `MovementState`, `StanceState`, `GaitState`
- Состояние траверсала из `TraversalComponent`

---

### `UProtagonistMovementComponent` — движение

**Файлы:** [ProtagonistMovementComponent.h](Source/DesertGame/Public/ProtagonistCharacter/ProtagonistMovementComponent.h) / [.cpp](Source/DesertGame/Private/ProtagonistCharacter/ProtagonistMovementComponent.cpp)

Кастомный компонент движения (в базовом состоянии — расширение для будущей логики).

---

### `ProtagonistCharacterTypes.h` — типы персонажа

**Файл:** [ProtagonistCharacterTypes.h](Source/DesertGame/Public/ProtagonistCharacter/ProtagonistCharacterTypes.h)

Содержит enum-ы `EGaitState`, `EMovementState`, `EStanceState`.

---

## Система траверсала

### `UTraversalComponent` — траверсал препятствий

**Файлы:** [TraversalComponent.h](Source/DesertGame/Public/Traversal/TraversalComponent.h) / [.cpp](Source/DesertGame/Private/Traversal/TraversalComponent.cpp)

Компонент, реализующий 5-шаговое определение препятствия и проигрывание соответствующего монтажа.

**Типы действий (`ETraversalAction`):**
`None`, `Vault`, `MantleLow`, `MantleHigh`, `ClimbOver`, `LedgeGrab`, `WallClimb`

**Алгоритм обнаружения (5 шагов):**

| Шаг | Метод | Описание |
|---|---|---|
| 1 | `ForwardTrace()` | 3 трейса на высотах 50/90/130 cm — поиск стены |
| 2 | `HeightTrace()` | Трейс сверху вниз — определение высоты уступа |
| 3 | `DepthTrace()` | Определение толщины препятствия (тонкое / толстое) |
| 4 | `RoomCheck()` | Capsule trace над препятствием — есть ли место для посадки |
| 5 | `ClassifyObstacle()` | Определение типа действия по высоте, глубине и наличию места |

**Ключевые пороги (`FTraversalTraceSettings`):**

| Параметр | Значение |
|---|---|
| MaxForwardDistance | 250 cm |
| MaxObstacleHeight | 350 cm |
| MinObstacleHeight | 50 cm |
| VaultMaxDepth | 50 cm |
| LowObstacleMaxHeight | 100 cm |
| HighObstacleMaxHeight | 250 cm |
| CapsuleCheckRadius | 34 cm |
| ForwardTraceRadius | 15 cm |

**Исполнение:** устанавливает MotionWarp-цель, переключает персонажа в `MOVE_Flying`, проигрывает монтаж, игнорирует коллизию препятствия на время анимации, восстанавливает движение по окончании.

**Debug-режим:** `bDebugDraw` — цветовая визуализация всех трейсов.

---

### `TraversalTypes.h` — структуры траверсала

**Файл:** [TraversalTypes.h](Source/DesertGame/Public/Traversal/TraversalTypes.h)

Определяет `ETraversalAction`, `FTraversalCheckResult` (результат замеров), `FTraversalTraceSettings` (конфигурация).

---

## Боевая система

### `UCombatComponent` — боевой компонент

**Файлы:** [CombatComponent.h](Source/DesertGame/Public/Combat/CombatComponent.h) / [.cpp](Source/DesertGame/Private/Combat/CombatComponent.cpp)

Компонент с Tick. Управляет комбо-атаками, блоком и уклонением.

**Состояния (`ECombatState`):** `Idle`, `Attacking`, `Blocking`, `Dodging`

**Комбо-система:**
- Массив имён секций: `Attack1`, `Attack2`, `Attack3`, `Attack4`
- `PendingNextCombo` — буферизует ввод во время окна комбо
- `ComboIndex` — текущий удар в цепочке
- При закрытии окна через `AnimNotify` автоматически переходит к следующему удару, если был буферизован

**Блок:**
- `StartBlock()` — запускает монтаж блока
- `StopBlock()` — останавливает монтаж с блендом 0.1 s

**Уклонение (`EDodgeDirection`):** `Forward`, `Backward`, `Left`, `Right`
- `RequestDodge(Direction)` — проигрывает монтаж направленного уклонения
- Дополнительное скользящее смещение поверх корневого движения: **200 cm за 0.3 s**

---

### `UAnimNotifyState_ComboWindow` — окно комбо

**Файлы:** [AnimNotifyState_ComboWindow.h](Source/DesertGame/Public/Combat/AnimNotifyState_ComboWindow.h) / [.cpp](Source/DesertGame/Private/Combat/AnimNotifyState_ComboWindow.cpp)

AnimNotifyState в монтаже атаки.
- `NotifyBegin()` → `OpenComboWindow()` — разрешает приём следующего инпута
- `NotifyEnd()` → `CloseComboWindow()` — если был буферизован ввод, переходит к следующему удару

---

## Система врагов (AI)

### `AEnemyCharacter` — базовый враг

**Файлы:** [EnemyCharacter.h](Source/DesertGame/Public/Enemy/EnemyCharacter.h) / [.cpp](Source/DesertGame/Private/Enemy/EnemyCharacter.cpp)

Базовый класс врага. AutoPossess — `AEnemyAIController`.

| Параметр | Значение |
|---|---|
| PatrolSpeed | 200 cm/s |
| ChaseSpeed | 450 cm/s |
| PatrolWaitTime | 3 s |
| AttackRange | 200 cm |

`GetNextPatrolPoint()` — циклически возвращает следующую точку из `PatrolPoints`.

---

### `AEnemyAIController` — AI-контроллер

**Файлы:** [EnemyAIController.h](Source/DesertGame/Public/Enemy/EnemyAIController.h) / [.cpp](Source/DesertGame/Private/Enemy/EnemyAIController.cpp)

Управляет Behavior Tree, Blackboard и AI Perception.

**Зрение (UAISenseConfig_Sight):**

| Параметр | Значение |
|---|---|
| SightRadius | 1800 cm |
| LoseSightRadius | 2500 cm |
| PeripheralVisionAngle | 90° (полукон с каждой стороны) |
| AutoSuccessRange | 600 cm (видит через препятствия вблизи) |

**Ключи Blackboard:**
- `TargetActor` — текущая цель (игрок)
- `LastKnownLocation` — последняя известная позиция игрока

**Debug:** при `ENABLE_DRAW_DEBUG` отрисовывает конус обзора (красный — цель есть, зелёный — поиск, жёлтый круг — зона AutoSuccess).

---

### BT Tasks и Services

| Класс | Файлы | Назначение |
|---|---|---|
| `UBTTask_FindNextPatrolPoint` | [.h](Source/DesertGame/Public/Enemy/BTTask_FindNextPatrolPoint.h) / [.cpp](Source/DesertGame/Private/Enemy/BTTask_FindNextPatrolPoint.cpp) | Получает следующую точку патруля, записывает в BB `PatrolLocation` |
| `UBTTask_EnemyAttack` | [.h](Source/DesertGame/Public/Enemy/BTTask_EnemyAttack.h) / [.cpp](Source/DesertGame/Private/Enemy/BTTask_EnemyAttack.cpp) | Запускает монтаж атаки, ждёт конца анимации через TickTask |
| `UBTTask_InvestigateLocation` | [.h](Source/DesertGame/Public/Enemy/BTTask_InvestigateLocation.h) / [.cpp](Source/DesertGame/Private/Enemy/BTTask_InvestigateLocation.cpp) | Идёт к `LastKnownLocation`, ждёт 3 s, очищает точку; прерывается при повторном обнаружении |
| `UBTService_UpdateSpeed` | [.h](Source/DesertGame/Public/Enemy/BTService_UpdateSpeed.h) / [.cpp](Source/DesertGame/Private/Enemy/BTService_UpdateSpeed.cpp) | Каждые 0.25 s выставляет ChaseSpeed / PatrolSpeed по наличию цели |

---

## Конфигурация сборки

**Файл:** [DesertGame.Build.cs](Source/DesertGame/DesertGame.Build.cs)

| Тип | Зависимости |
|---|---|
| Public | Core, CoreUObject, Engine, InputCore, EnhancedInput, AIModule, GameplayTasks, NavigationSystem |
| Private | MotionWarping, AnimGraphRuntime |

---

## Контент-ассеты

### Ghost (`Content/enemies/ghost/`)

| Ассет | Тип |
|---|---|
| `ghost.uasset` | Skeletal Mesh |
| `ghost_PhysicsAsset.uasset` | Physics Asset |
| `ABP_Ghost.uasset` | Animation Blueprint |
| `BT_Ghost.uasset` | Behavior Tree |
| `BB_Ghost.uasset` | Blackboard |
| `BP_Ghost.uasset` | Character Blueprint |
| `AM_Ghost_Attack.uasset` | Anim Montage (атака) |
| `animations/Anim_ghost_idle/walk/attack/die` | Animation Sequences |
| `magic_ball.uasset`, `scythe.uasset` | Weapon / VFX Meshes |

### Hyena (`Content/enemies/hyena/`)

| Ассет | Тип |
|---|---|
| `hyena.uasset` | Skeletal Mesh |
| `hyena_Skeleton.uasset` | Skeleton |
| `hyena_PhysicsAsset.uasset` | Physics Asset |
| `Anim_hyena_Idle/Walk/Run/Atk1/Atk2/Atk3` | Animation Sequences |

### Sandworm (`Content/enemies/sandworm/`)

| Ассет | Тип |
|---|---|
| `sandworm.uasset` | Skeletal Mesh |
| `sandworm_Skeleton.uasset` | Skeleton |
| `sandworm_PhysicsAsset.uasset` | Physics Asset |
| `Anim_sandworm_bite/stun` | Animation Sequences |
| `sand_ring_VFX.uasset` | VFX |

---

## Иерархия классов

```
ACharacter
├── AProtagonistCharacter          — игрок
└── AEnemyCharacter                — базовый враг

UActorComponent
├── UTraversalComponent            — преодоление препятствий
└── UCombatComponent               — бой

UAnimInstance
└── UProtagonistAnimInstance       — параметры анимации игрока

AAIController
└── AEnemyAIController             — управление AI

UBTTaskNode
├── UBTTask_FindNextPatrolPoint    — следующая точка патруля
├── UBTTask_EnemyAttack            — атака
└── UBTTask_InvestigateLocation    — расследование позиции

UBTService
└── UBTService_UpdateSpeed         — скорость по наличию цели

UAnimNotifyState
└── UAnimNotifyState_ComboWindow   — окно комбо
```
