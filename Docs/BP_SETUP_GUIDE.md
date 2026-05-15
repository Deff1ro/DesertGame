# DesertGame — Гайд для Blueprint-разработчика

Документ описывает, **что и как настраивается в редакторе** (Blueprints, Data Assets, Widgets), какие C++-классы за этим стоят и куда их подключать. Сам код не объясняется — речь только об интерфейсе для дизайнера/BP-разработчика.

Карта проекта:
- **Стартовая карта (билд):** `Content/Maps/L_MainMenu.umap`
- **Стартовая карта (редактор):** `Content/Maps/BASE_Map.umap`
- **GameMode по умолчанию:** `Content/Characters/ProtagonistCharacter/GM_Base`
- **Module:** `DesertGame` (`Source/DesertGame/`)

Все C++-классы из модуля доступны как родители для BP. В тексте ниже C++-имя пишется в `моноширинном`, а ассет — `BP_Foo` / `WBP_Foo` / `DA_Foo`.

---

## Содержание

1. [Игрок и движение](#1-игрок-и-движение)
2. [Камера](#2-камера)
3. [Боёвка (меч/блок/уклонение)](#3-боёвка)
4. [Инструменты и добыча ресурсов](#4-инструменты-и-добыча-ресурсов)
5. [Параллельная анимация (traversal — вольты/менты)](#5-traversal-вольтыменты)
6. [Атрибуты: HP / Stamina / Hunger / Thirst](#6-атрибуты)
7. [День/Ночь и среда](#7-деньночь-и-среда)
8. [Враги](#8-враги)
9. [AI: восприятие, BT, BB](#9-ai-восприятие-bt-bb)
10. [Червь (Sandworm)](#10-червь-sandworm)
11. [Ресурсы (деревья, камни)](#11-ресурсы)
12. [Предметы, инвентарь, хотбар, одежда](#12-предметы-и-инвентарь)
13. [Крафтинг и рецепты](#13-крафтинг)
14. [UI / HUD](#14-ui--hud)
15. [Сохранение / загрузка](#15-сохранениезагрузка)
16. [Звук](#16-звук-сводная-таблица)
17. [Checklist: «Я добавил нового …»](#17-checklist)

---

## 1. Игрок и движение

**C++ класс:** `AProtagonistCharacter` → **BP:** `Content/Characters/ProtagonistCharacter/BP_ProtagonistCharacter`

### Что задаётся в BP

**Movement | Settings:**
- `WalkSpeed` (200) — скорость ходьбы (Walk).
- `RunSpeed` (500) — обычный бег.
- `SprintSpeed` (800) — спринт (по Shift).
- `CrouchSpeedMultiplier` (0.5) — множитель скорости в приседе.

**Camera:** компоненты `CameraBoom` (`UPlayerSpringArmComponent`) и `FollowCamera`. На `CameraBoom` настраивается длина, лаг, угол. **Не меняйте класс CameraBoom**, иначе пропадёт фикс, не дающий камере «нырять» сквозь врагов и предметы (см. [Камера](#2-камера)).

**Components (визуал):**
- `EquippedToolMesh` (StaticMeshComponent) — меш инструмента в руке. Прикрепляется к сокету `ToolAttachSocket` (по умолчанию `hand_r_socket`) **в BeginPlay**, так что сокет должен реально быть на скелете руки.
- `CloakMesh` (SkeletalMeshComponent) — оверлей плаща, использует Leader Pose от тела. На него в BP назначайте *Skeletal Mesh* плаща (тот же скелет, что у тела).

**Input (UInputAction-ассеты):** все слоты `MoveAction / LookAction / JumpAction / SprintAction / CrouchAction / AttackAction / BlockAction / DodgeAction / SaveGameAction / InteractAction / ToggleInventoryAction / HotbarSlot1..4Action` и `DefaultMappingContext` — заполните соответствующими ассетами из `Content/Input/Protagonist/`:
- `IMC_Default` → `DefaultMappingContext`
- `IA_Move / IA_Look / IA_Jump / IA_Sprint / IA_Crouch` → одноимённые слоты
- `IA_Attack` → `AttackAction`, `IA_Block` → `BlockAction`, `IA_Dodge` → `DodgeAction`
- `IA_Save` → `SaveGameAction`, `IA_Interact` → `InteractAction`, `IA_ToggleInventory` → `ToggleInventoryAction`
- `IA_Cell1..Cell4` → `HotbarSlot1..4Action`

**UI:**
- `HubWidgetClass` — выпадающий список под класс окна инвентаря (`WBP_Inventory`).

**Audio:**
- `FootstepSound` (USoundBase) — один звук шага. Триггерится либо AnimNotify-ом на анимациях ходьбы/бега, либо вручную из BP вызовом `PlayFootstepSound`.

**BlueprintImplementableEvent (имплементируются в BP_ProtagonistCharacter):**
- `OnHeatSufferChanged(bool bSuffering)` — вызывается, когда игрок начинает / перестаёт страдать от жары. Используйте, чтобы показать/скрыть PNG-оверлей.
- `OnColdSufferChanged(bool bSuffering)` — то же для холода.

### Геттеры для BP

`GetAttributeComponent`, `GetInventoryComponent`, `GetToolUseComponent`, `GetStanceState`, `IsSufferingFromHeat`, `IsSufferingFromCold`.

### Поведение по умолчанию

- Бить кулаками **нельзя** — `OnAttackInput` атакует только если в активном слоте хотбара лежит инструмент. Меч → `CombatComponent`, топор/кирка → `ToolUseComponent`.
- Одежда применяется автоматически из слота `ClothSlot`. См. [§ 12](#12-предметы-и-инвентарь).

---

## 2. Камера

**C++ класс:** `UPlayerSpringArmComponent` (наследник `USpringArmComponent`).

Используется как тип компонента `CameraBoom` у `BP_ProtagonistCharacter`. Никаких UPROPERTY-настроек собственных не добавляет — наследует все стандартные. Особенность: каждый кадр перед расчётом своего положения вынуждает все экземпляры `AEnemyCharacter`, `AItemActor`, `AResourceNode` игнорировать `ECC_Camera`. Это лечит баг с «нырянием» камеры внутрь меша игрока, когда враг подходит вплотную.

Просто оставляйте `CameraBoom` типом `PlayerSpringArmComponent`, не меняйте на стандартный.

---

## 3. Боёвка

**C++ класс:** `UCombatComponent` (`ClassGroup=Custom`, Blueprint-spawnable). Уже добавлен в `BP_ProtagonistCharacter`.

### Combat | Settings

- `ComboMontage` (UAnimMontage) — общий монтаж со всеми атаками меча в разных секциях.
- `ComboSectionNames` (TArray<FName>) — имена секций в порядке комбо (`Attack1, Attack2, ...`).
- `BlockMontage` — анимация блока.
- `DodgeMontages` (TMap<EDodgeDirection, UAnimMontage>) — четыре анимации уклонения: Forward / Backward / Left / Right.
- `DodgeExtraDistance` (200 см) — сколько дополнительно проехать сверх root motion.
- `DodgeSlideDuration` (0.3 с) — длительность доезда.

### Combat | Sword (гитбокс)

Сабля бьёт капсульным свипом перед игроком, ищет `AEnemyCharacter` и `AWormEnemy`.

- `SwordCapsuleRadius` (50) / `SwordCapsuleHalfHeight` (90) — размер капсулы.
- `SwordForwardReach` (120) — на сколько вперёд от персонажа сместить центр капсулы.
- `SwordVerticalOffset` — поднять/опустить капсулу по Z.
- `bDebugDrawSwordHit` — нарисовать капсулу на 0.5 с.

**Урон** не настраивается тут — берётся из `ToolDamage` экипированного меча (см. `UItemDataAsset`).

### Combat | Sword | Audio

- `SwordSwingSound` — звук взмаха (триггерится каждым `UAnimNotify_SwordHit`).
- `SwordHitSound` — звук попадания.

### Anim Notify ассеты для монтажа меча

В Combo Montage в `ComboMontage` нужно расставить:
1. **`AnimNotify_SwordHit`** — на тех кадрах, где меч реально попадает. Вызывает `PerformSwordHit()`.
2. **`AnimNotifyState_ComboWindow`** — на отрезке кадров, в течение которого нажатие *Attack* откроет следующую секцию комбо.

### Anim Notify для блока

`BlockMontage` обычно зацикленный (loop section) — это просто стойка, без notify.

### Уклонение

`OnDodgeInput` сам решает направление по WASD относительно камеры. Затем вызывает `RequestDodge(Direction, WorldSlideDirection)`. Если `WorldSlideDirection` нулевой — используется направление актёра (legacy).

---

## 4. Инструменты и добыча ресурсов

**C++ класс:** `UToolUseComponent`. Уже добавлен в `BP_ProtagonistCharacter`.

### Tool | Animations

- `ToolMontages` (TMap<EToolType, UAnimMontage>) — на каждый тип инструмента (Axe / Pickaxe / Sword — но Sword обычно идёт через `CombatComponent`) свой монтаж замаха.

### Tool | Trace

- `HitTraceDistance` (180) — длина сферического свипа.
- `HitTraceRadius` (50) — радиус свипа.
- `bDebugDraw` — нарисовать сферу попадания.

### Tool | Audio

- `ToolSwingSounds` (TMap<EToolType, USoundBase>) — звук замаха на каждый тип инструмента.
- `ToolHitSound` — общий звук попадания инструмента.

### Anim Notify для монтажа инструмента

В каждом `ToolMontages[X]` ставьте **`UAnimNotify_ToolHit`** на кадре удара — он вызовет `PerformToolHit()`. Свип ищет:
- `AResourceNode` (см. § 11) — добывает ресурс, если тип инструмента подходит;
- `AEnemyCharacter` и `AWormEnemy` — наносит `ToolDamage` (из `UItemDataAsset` экипированного инструмента).

### Логика выбора инструмента

Если в активном слоте хотбара предмет с `ToolType == Sword` — атаку обрабатывает `CombatComponent`. Иначе атаку обрабатывает `ToolUseComponent` (Axe/Pickaxe). Без инструмента — атаки нет.

---

## 5. Traversal (вольты/менты)

**C++ класс:** `UTraversalComponent` (Blueprint-spawnable). На игроке.

### Traversal | Settings

- `TraceSettings` (FTraversalTraceSettings) — все геометрические пороги:
  - `MaxForwardDistance` (250), `MaxObstacleHeight` (350), `MinObstacleHeight` (50)
  - `TopTraceStartHeight` (400), `DepthCheckDistance` (80)
  - `VaultMaxDepth` (50), `LowObstacleMaxHeight` (100)
  - `ClimbOverMinHeight` (90), `HighObstacleMaxHeight` (250)
  - `CapsuleCheckRadius` (34), `CapsuleCheckHalfHeight` (88), `ForwardTraceRadius` (15)
  - `ClimbOverLandingOffset` (100), `MantleHighZOffset` (75)
- `TraversalMontages` (TMap<ETraversalAction, UAnimMontage>) — анимации для `Vault / MantleLow / MantleHigh / ClimbOver / LedgeGrab / WallClimb`.
- `WarpTargetName` (`TraversalTarget`) — имя цели для Motion Warping. Должно совпадать с FName в анимациях.

### Traversal | Debug

- `bDebugDraw`, `DebugDrawDuration` (2 с).

### Обязательно

На игроке должен быть `UMotionWarpingComponent` (он уже есть в `AProtagonistCharacter`). Анимациям в `TraversalMontages` нужны Motion Warping Anim Notifies с тем же `WarpTargetName`.

---

## 6. Атрибуты

**C++ класс:** `UAttributeComponent` (Blueprint-spawnable). Уже добавлен в `BP_ProtagonistCharacter`.

### Health

- `MaxHealth` (100).

### Stamina

- `MaxStamina` (100), `StaminaDrainRate` (20/с), `StaminaRegenRate` (25/с)
- `StaminaRegenDelay` (1.5 с) — задержка перед началом регена.
- `StaminaMinToSprint` (10) — минимум стамины, чтобы начать спринтить.

### Hunger / Thirst

- `MaxHunger` (100), `HungerDrainPerSecond` (0.333 → 100% за 5 минут).
- `MaxThirst` (100), `ThirstDrainPerSecond` (0.555 → 100% за 3 минуты).

### Events (BlueprintAssignable)

`OnHealthChanged(float, float)`, `OnStaminaChanged`, `OnHungerChanged`, `OnThirstChanged`, `OnDeath`, `OnStaminaDepleted`. HUD на них подписан автоматически.

### API для BP

`ApplyDamage(float)`, `Heal(float)`, `ConsumeHunger / RestoreHunger`, `ConsumeThirst / RestoreThirst`. Множители среды устанавливаются автоматически из `ADayNightCycleManager` (см. § 7).

---

## 7. День/Ночь и среда

**C++ класс:** `ADayNightCycleManager`. **Поместите ровно один экземпляр** на карту (можно `Place Actor` напрямую без BP-обёртки, или сделать BP-наследника).

### Cycle

- `DayDurationSeconds` (600 = 10 мин), `NightDurationSeconds` (300 = 5 мин).
- `StartingPhase` (Day / Night).

### Cycle | Sun

- `SunLight` (ADirectionalLight) — **назначьте Directional Light со сцены**. Менеджер крутит его pitch.
- `SunriseRotation` (по умолчанию `-10, 0, 0`) — поворот солнца на восходе.
- `SunsetRotation` (`-170, 0, 0`) — поворот на закате. **Pitch должен монотонно убывать** от Sunrise до Sunset (через -90 = зенит).

### Gameplay | Day / Night

- `DayThirstMultiplier` (1.5) — днём жажда уходит x1.5.
- `NightHungerMultiplier` (1.5) — ночью голод уходит x1.5.
- `DayUnprotectedHealthDrainPerSecond` (2) — урон HP/сек днём, если **нет** одежды с `Protection=Heat`.
- `NightUnprotectedHealthDrainPerSecond` (2) — урон HP/сек ночью, если **нет** одежды с `Protection=Cold`.

### Audio

- `DayMusic / NightMusic` — амбиент. Менеджер сам кроссфейдит за `MusicFadeSeconds` (1.5 с) при смене фазы.

### Events

- `OnPhaseChanged(EDayNightPhase NewPhase)` — диспетчер для BP-логики.

### Получить из BP

`Get(WorldContext)` — статический blueprint-pure — возвращает менеджер с уровня (берёт первый найденный TActorIterator-ом).

---

## 8. Враги

**C++ класс:** `AEnemyCharacter` → BP: `BP_Hyena` (Content/enemies/hyena/), `BP_Ghost` (Content/enemies/ghost/), любой свой моб.

### Enemy | Health

- `MaxHealth` (100), `DeathLingerTime` (0) — на сколько труп остаётся, чтобы успела доиграть Death-анимация (0 = удалить сразу).

### Enemy | Loot

Логика дропа **один в один** с `AResourceNode`: разлетается с импульсом, включается физика.

- `LootTable` (TArray<FEnemyLootEntry>) — массив записей. Каждая запись:
  - `Item` (UItemDataAsset) — что выпадает.
  - `DropMin` / `DropMax` — диапазон количества.
- `DropScatterRadius` (100) — радиус разброса. Внутри: `min(Radius * 0.25, 25)` см.
- `DropSpawnHeight` (50) — на какой высоте над землёй спавнятся дропы.
- `DropActorClass` (TSubclassOf<AItemActor>) — **обязательно назначьте**, иначе ничего не упадёт. Обычно это `BP_Item_Wood`, `BP_Item_Stone` или базовый `AItemActor`-наследник.

### AI | Patrol

- `PatrolPoints` (TArray<AActor*>) — точки патруля, расставляются **на самой сцене**, не в Defaults. Можно ставить пустые `Target Point` актёры.
- `PatrolWaitTime` (3 с), `PatrolSpeed` (200), `ChaseSpeed` (450).

### AI | Combat

- `AttackRange` (200) — дистанция, на которой AI начинает атаковать.
- `AttackMontage` (UAnimMontage) — анимация атаки.

### AI

- `BehaviorTree` (UBehaviorTree) — назначьте `BT_Ghost` (общее BT для всех мобов в проекте).

### AttackComponent

`UEnemyAttackComponent` уже создан в конструкторе. На нём настраивайте:

- `AttackDamage` (10) — урон по игроку.
- `AttackCapsuleRadius` / `AttackCapsuleHalfHeight` (60 / 90).
- `AttackForwardReach` (120) — насколько вынести центр капсулы вперёд.
- `AttackVerticalOffset` (0).
- `bDebugDraw`.
- `AttackSwingSound`, `AttackHitSound`.

### Anim Notify для AttackMontage

Поставьте `UAnimNotify_EnemyAttackHit` на кадре удара — вызовет `PerformAttackHit()` у компонента.

### Events для BP

`OnDied()` — диспетчер.

---

## 9. AI: восприятие, BT, BB

### Контроллер

**C++ класс:** `AEnemyAIController`. Авто-поссессит `AEnemyCharacter`. Менять класс контроллера в BP моба **не нужно**.

Настройки в C++ — отредактировать на `BP_<Enemy>` нельзя (`EditDefaultsOnly` стоит, но контроллер свой, не сам моб). Если нужно поменять — наследовать `AEnemyAIController` BP-обёрткой и подменить в моб-BP. Поля:

- `InvestigationTimeout` (6 с) — сколько секунд преследовать после потери цели.
- `ProximityRadius` (500) — радиус, в котором игрок виден «спиной» (Tick-based).
- `SpotPlayerSound` — звук «заметил».
- `CombatMusic` — отдельный 2D-трек на время боя, останавливается при потере цели.

### BT / BB

**Готовые ассеты:**
- `Content/enemies/ghost/BT_Ghost` — общее BT (используется и для гиены).
- `Content/enemies/ghost/BB_Ghost` — блекборд.

**Ключи Blackboard (обязательные имена):**
- `TargetActor` — `Object/AActor`. Игрок, если виден.
- `LastKnownLocation` — `Vector`. Куда идти расследовать.
- `HomeLocation` — `Vector`. Точка возврата к патрулю.

### BT-Tasks/Services (C++)

- `UBTTask_FindNextPatrolPoint` — берёт следующую точку из `AEnemyCharacter::PatrolPoints`, кладёт в BB.
- `UBTTask_EnemyAttack` — поворачивает моба к `TargetActor`, играет `AttackMontage`, ждёт окончания.
- `UBTTask_InvestigateLocation` — идёт в `LastKnownLocation`, ждёт `WaitDuration` (3 с по умолч., настраивается на ноде в BT), `AcceptanceRadius` (100).
- `UBTService_UpdateSpeed` — каждый тик подстраивает `Max Walk Speed` (PatrolSpeed / ChaseSpeed) в зависимости от наличия цели.

### Observer Aborts

В BT на узлах, проверяющих `TargetActor`, должен стоять *Observer Aborts: Both* — иначе моб не выйдет из боя при потере цели.

---

## 10. Червь (Sandworm)

**C++ класс:** `AWormEnemy` (наследник `AActor`, не `AEnemyCharacter`!) → BP: `Content/enemies/sandworm/BP_Worm`.

### Components

- `MeshComponent` (USkeletalMeshComponent) — назначьте sandworm skeletal mesh + Anim BP `ABP_Worm`.
- `DetectionTrigger` (UCapsuleComponent) — триггер «жертва над поверхностью». Радиус/высоту настройте под локацию.
- `KillCapsule` (UCapsuleComponent) — капсула, в которой червь убивает в момент удара. По умолчанию отключена, включается на фазе Attacking. Подгоните под визуальный размер пасти.

### Worm | Timing

- `TelegraphDelay` (2 с) — задержка между «я тебя засёк» и началом подъёма (предупреждение для игрока).
- `AttackDuration` (5 с) — сколько червь стоит наверху.

### Worm | Motion

- `RiseDistance` (200 см) — насколько меш поднимается над `BuriedLocation`.
- `RiseDuration` (0.6 с) — длительность подъёма (плавно через smoothstep).
- `BurrowDuration` (0.6 с) — длительность спуска.

### Worm | Combat

- `StrikeDamage` (10000) — урон в момент удара. По умолчанию очень большой → мгновенная смерть.
- `AttackMontage` — основной монтаж удара.
- `RiseMontage` — зацикленный (циклится через `Montage_SetEndDelegate`) монтаж подъёма.

### Worm | Health

- `MaxHealth` (100), `DeathLingerTime` (0).
- Урон по червю принимается тем же `TakeDamageAmount(float)`, что и у обычного врага — поэтому и меч, и инструменты могут добивать.

### Worm | Loot

Идентично `AEnemyCharacter`:
- `LootTable` (TArray<FEnemyLootEntry>), `DropScatterRadius` (100), `DropSpawnHeight` (50), `DropActorClass`.

### Worm | FX

- `CameraShakeClass` (TSubclassOf<UCameraShakeBase>) — тряска на локальном игроке от телеграфа до подъёма (потом останавливается).
- `CameraShakeScale` (1.0).

### Worm | Audio

- `EmergeSound` — момент начала подъёма (рык).
- `StrikeHitSound` — попадание удара.

### Worm | Debug

- `bDebugDrawKillCapsule`.

### Размещение на сцене

Просто перетащите `BP_Worm` на карту, опустите по Z так, чтобы меш был под землёй. На `BeginPlay` червь запоминает `BuriedLocation` и поднимается ровно на `RiseDistance` вверх. **Не двигайте `MeshComponent` относительно root** в BP — у червя нет навмеша, его «движение» это интерполяция меша.

---

## 11. Ресурсы

**C++ класс:** `AResourceNode` → BP: `Content/Items/Objects/BP_ResourceNode` (базовый) или `BP_Tree` (вариант для дерева).

### Resource

- `NodeData` (UResourceNodeData) — назначьте Data Asset, см. ниже.

### Resource | Drop

- `DropActorClass` (TSubclassOf<AItemActor>) — **обязательно**, иначе дропы не появятся. Обычно `BP_Item_Wood / BP_Item_Stone`.

### Data Asset `UResourceNodeData`

Создаётся через *Right click → Miscellaneous → Data Asset → ResourceNodeData*. Примеры: `DA_Tree`.

- **ResourceNode:**
  - `DisplayName` — отображаемое имя.
  - `Mesh` (UStaticMesh) — меш для ноды (применяется в BeginPlay поверх того, что в `BP_ResourceNode`).
- **ResourceNode | Tool:**
  - `RequiredTool` (EToolType) — какой нужен инструмент. Удар «не тем» инструментом → лог + игнор.
- **ResourceNode | Health:**
  - `MaxHealth` (5) — сколько ударов выдержит.
- **ResourceNode | Drop:**
  - `DropItem` (UItemDataAsset) — что выпадает.
  - `DropMin` / `DropMax` — диапазон количества.
  - `DropScatterRadius` (100) — радиус разлёта.
  - `DropSpawnHeight` (50).

### Логика разлёта (общая для ResourceNode и Enemy)

- На каждый дроп: случайный угол `[0..2π]`, латеральный импульс `[150..300]`, вертикальный `[250..400]`, вращение случайное.
- Спавн идёт на `Origin + LateralDir * RandRange(0..min(Radius*0.25, 25))`. Это нужно, чтобы предметы не появлялись друг в друге.

---

## 12. Предметы и инвентарь

### UItemDataAsset

Создаётся: *Right click → Miscellaneous → Data Asset → ItemDataAsset*. Примеры: `DA_Item_Wood`, `DA_Sword`, `DA_Pickaxe`, `DA_Coconut`, `DA_Cloack`, `DA_Item_StoneAxe`.

**Item:**
- `ItemType` (EItemType: None/Wood/Metal/Stone) — служебная категория.
- `Category` (EItemCategory: Generic/Cloth) — нужна для слот-ограничений (одежда).
- `DisplayName`, `Icon` (UTexture2D), `MaxStackSize` (1..10).

**Item | World:**
- `WorldMesh` (UStaticMesh) — меш дропа на земле.

**Item | Tool:**
- `ToolType` (None/Sword/Axe/Pickaxe) — тип инструмента. None = это не инструмент.
- `EquipMesh` (UStaticMesh) — меш, который вешается на сокет в руке. Если пусто — берётся `WorldMesh`.
- `ToolDamage` (1) — урон по живой цели (Enemy/Worm) или ResourceNode HP за удар.

**Item | Consumable** (видны только если `bIsConsumable=true`):
- `bIsConsumable` (bool) — главный флаг.
- `HungerRestore`, `ThirstRestore`, `HealthRestore` (float) — что восстановит.

Расходники потребляются вызовом `InventoryComponent->ConsumeSelectedItem()`. Привязано к Input на стороне BP_Character — настройте сами или используйте существующую механику.

**Item | Cloth** (используется только когда `Category=Cloth`):
- `ClothEquipMode` — **Overlay** (плащ — кладётся в `CloakMesh`) или **BodySwap** (заменяет тело целиком).
- `ClothSkeletalMesh` (USkeletalMesh) — **тот же скелет, что и у тела игрока**.
- `Protection` (None / Heat / Cold) — от какой среды защищает.

### BP_Item / AItemActor

**C++ класс:** `AItemActor` → BP: `BP_Item_Wood`, `BP_Item_Stone`, `BP_Coconut`, `BP_Sword`. Используется и как мировой предмет (placed-on-map), и как класс для спавна дропов.

- `ItemData` (UItemDataAsset) — что это за предмет.
- `Quantity` (1+) — сколько.
- `PickupSound` — звук подбора.

В сцене (placed instance) поднимать игроку: подойти к зоне `PickupZone` (radius 150) и нажать Interact. Если `BP_Item` уже подбирался в сохранении — он автоматически уничтожится на `BeginPlay`.

### UInventoryComponent

Уже на игроке. Настройки:
- `InventorySize` (16) — основная сумка.
- `HotbarSize` (4) — хотбар.

Слоты в рантайме:
- `InventorySlots[16]`, `HotbarSlots[4]`, `ClothSlot` (один).

**Events (BlueprintAssignable):**
- `OnInventoryChanged(int32 SlotIndex)` — изменилась ячейка сумки.
- `OnHotbarChanged(int32 SlotIndex)` — изменилась ячейка хотбара.
- `OnHotbarSelectionChanged(int32 OldIndex, int32 NewIndex)` — игрок выбрал другой слот хотбара.
- `OnSelectedHotbarChanged(int32 NewIndex)` — упрощённый вариант, **слушает `AProtagonistCharacter` для смены меша в руке**.
- `OnClothChanged()` — слот одежды изменился (используется для перерисовки плаща/тела).

**API для BP:**
- `AddItem(ItemData, Qty)` → возвращает остаток, который не влез.
- `MoveItem(From, To)` — перенос между любыми контейнерами. Проверяет `bRestrictByCategory` у целевого UI-слота.
- `RemoveFromSlot(SlotRef, Amount)`, `RemoveItems(Item, Amount)`, `GetItemTotalCount`.
- `CanCraft(Recipe)` / `TryCraft(Recipe)`.
- `SelectHotbarSlot(Index)` — нажатие 1/2/3/4, повторное нажатие снимает выбор (INDEX_NONE).
- `GetSelectedItem()`, `ConsumeSelectedItem()`.

### Одежда / экипировка

Обрабатывается полностью в `AProtagonistCharacter::ApplyClothFromInventory`:
- `ClothEquipMode=Overlay` → `CloakMesh` получает `ClothSkeletalMesh`, тело возвращается к `DefaultBodyMesh`.
- `ClothEquipMode=BodySwap` → меш тела заменяется на `ClothSkeletalMesh`, `CloakMesh` очищается.

Чтобы предмет принимался в Cloth-слот, в `ItemDataAsset` поставьте `Category=Cloth`, и на UI-ячейке Cloth включите `bRestrictByCategory=true, AcceptedCategory=Cloth` (уже сделано в `WBP_ClothingCell`).

---

## 13. Крафтинг

### URecipeDataAsset

Создаётся: *Right click → Miscellaneous → Data Asset → RecipeDataAsset*. Примеры: `DA_Recipe_StoneAxe`, `DA_Stone_Sword`, `DA_Cloak_Recipe`.

- **Recipe:**
  - `DisplayName`, `RecipeIcon` (UTexture2D).
- **Recipe | Result:**
  - `ResultItem` (UItemDataAsset).
  - `ResultQuantity` (1+).
- **Recipe | Ingredients:**
  - `Ingredients` (TArray<FRecipeIngredient>) — список `{Item, Quantity}`.

### UCraftingWidget (WBP_Crafting)

В Defaults виджета:
- `AvailableRecipes` (TArray<URecipeDataAsset>) — какие рецепты показывать.
- `RecipeDetailsWidgetClass` (TSubclassOf<URecipeDetailsWidget>) — обычно `WBP_RecipeDetails`.

**BlueprintImplementableEvent:**
- `GatherRecipeCells()` — в BP реализуйте сбор ячеек рецептов в массив `RecipeCells` (через CommonElements / WrapBox).

### URecipeCellWidget (WBP_RecipeCell)

Реализует:
- `OnRecipeUpdated(Icon, bIsAvailable)` — BIE, перерисовать иконку/затенение.
- `OnRecipeCleared()` — BIE.

### URecipeDetailsWidget (WBP_RecipeDetails)

BindWidget'ы: `IngredientsList` (UVerticalBox), `TitleText` (UTextBlock).

Настройки цвета:
- `AvailableColor` (белый), `MissingColor` (красноватый).

---

## 14. UI / HUD

### HUD

**C++ класс:** `AProtagonistHUD` → BP: `BP_ProtagonistHUD` (Content/UI/).

- `MainHUDWidgetClass` (TSubclassOf<UMainHUDWidget>) — назначьте `WBP_MainHUD`.

В `GM_Base` укажите `HUD Class = BP_ProtagonistHUD`.

### Main HUD

**C++ класс:** `UMainHUDWidget` → `WBP_MainHUD`.

**BlueprintImplementableEvent:**
- `UpdateHealthBar(float Percent)`, `UpdateStaminaBar`, `UpdateHungerBar`, `UpdateThirstBar` — отрисуйте прогресс-бар.
- `GatherHotbarCells()` — соберите ячейки хотбара в `HotbarCells` (TArray<UInventoryCellWidget>).

**BlueprintReadWrite:** `HotbarCells`.

Подписки на `UAttributeComponent` и `UInventoryComponent` уже сделаны автоматически в `NativeConstruct` — ничего вручную биндить не надо.

### Inventory Hub

**C++ класс:** `UInventoryHubWidget` → `WBP_Inventory`.

**BindWidget (обязательные имена):**
- `TabSwitcher` (UWidgetSwitcher).
- `BagPage` (UBagWidget).
- `CraftingPage` (UCraftingWidget).

Открывается клавишей `ToggleInventoryAction` (на `AProtagonistCharacter`). При открытии:
- курсор → видимый, ввод → UI+Game,
- `bLookEnabled=false`.

### Bag

**C++ класс:** `UBagWidget` → `WBP_Bag`.

**BlueprintImplementableEvent:**
- `GatherCells()` — собрать `InventoryCells[16]` и `HotbarCells[4]`.

**BlueprintReadWrite:** `InventoryCells`, `HotbarCells`.

### Inventory Cell

**C++ класс:** `UInventoryCellWidget` → `WBP_InventoryCellWidget` (для сумки/хотбара) и `WBP_ClothingCell` (для одежды).

**Drag & Drop:**
- `DragVisualClass` (TSubclassOf<UUserWidget>) — обычно `WBP_DragVisual`.
- `bIsDraggable` (true).
- `bAcceptsDrop` (true).
- `bRestrictByCategory` (false по умолч., true для Cloth-слота).
- `AcceptedCategory` (Cloth) — куда категория сравнивается, если включено ограничение.

**BlueprintImplementableEvent:**
- `OnSlotDataUpdated(Icon, Quantity, bIsEmpty)` — основной хук перерисовки.
- `OnSelectionChanged(bIsSelected)` — подсветить выбранную ячейку.
- `OnIconUpdated(Icon)` — legacy, можно игнорировать.

### Drag Visual (WBP_DragVisual)

Подкласс `UUserWidget`. Получает данные через `UItemDragDropOperation` (поля `SourceSlot`, `DraggedItemData`, `DraggedQuantity`).

### Main Menu

**C++ класс:** `UMainMenuWidget` → `WBP_MainMenu`.

- `GameLevelName` (`L_Desert`) — на какой уровень переходить из меню. **Замените**, если ваш игровой уровень называется иначе (в проекте сейчас `BASE_Map`).
- Кнопки в BP биндите на `OnNewGameClicked / OnContinueClicked / OnExitClicked`.
- `HasSaveGame()` — пригодится, чтобы серой делать «Continue».

Стартовый GameMode для `L_MainMenu` — `Content/Core/BP_MainMenuGameMode`.

---

## 15. Сохранение/загрузка

**C++ классы:** `UDesertGameInstance` + `UDesertSaveGame`.

В `Project Settings → Maps & Modes → Game Instance Class` уже стоит `UDesertGameInstance`.

### Что сохраняется

- Уровень, позиция игрока, поворот, поза (Stance), временная метка.
- Полный snapshot `InventorySlots` и `HotbarSlots`.
- Список FName уже подобранных `AItemActor` (чтобы они не возрождались).

### API для BP (через GameInstance)

- `HasSaveGame()`, `SaveGame(Player)`, `LoadSaveGameData()`, `DeleteSaveGame()`.
- `RequestLoadOnNextLevel()`, `ShouldApplyLoadedSave()`, `ConsumeLoadRequest()`, `GetSavedLevelName()`.
- `RegisterPickedUpItem(FName)`, `IsItemPickedUp(FName)`, `ResetPickedUpItems()`.

Сохраняться можно из BP в любой момент: `Get Game Instance → Cast → Save Game(Player)`. В `BP_ProtagonistCharacter` есть `SaveGameAction`-биндинг.

---

## 16. Звук (сводная таблица)

| Где настроить | Поле | Когда играет |
|---|---|---|
| BP_ProtagonistCharacter | `FootstepSound` | AnimNotify / `PlayFootstepSound` |
| BP_ProtagonistCharacter → CombatComponent | `SwordSwingSound` | AnimNotify_SwordHit |
| BP_ProtagonistCharacter → CombatComponent | `SwordHitSound` | Свип попал по врагу |
| BP_ProtagonistCharacter → ToolUseComponent | `ToolSwingSounds[Type]` | AnimNotify_ToolHit |
| BP_ProtagonistCharacter → ToolUseComponent | `ToolHitSound` | Свип попал |
| BP_<Enemy> → AttackComponent | `AttackSwingSound` | AnimNotify_EnemyAttackHit |
| BP_<Enemy> → AttackComponent | `AttackHitSound` | Свип попал по игроку |
| AEnemyAIController (через BP-обёртку контроллера) | `SpotPlayerSound` | Игрок впервые замечен |
| AEnemyAIController | `CombatMusic` | Старт боя → потеря цели |
| BP_Worm | `EmergeSound` | Начало подъёма |
| BP_Worm | `StrikeHitSound` | Поймал жертву |
| BP_Item (любой AItemActor) | `PickupSound` | Подобран хоть один |
| ADayNightCycleManager | `DayMusic / NightMusic` | Кроссфейд на смене фазы (`MusicFadeSeconds`=1.5) |

---

## 17. Checklist

### Я добавил **новый предмет**:

1. *Create → Miscellaneous → Data Asset → ItemDataAsset* в `Content/Items/Data/` → `DA_<Name>`.
2. Заполните: `DisplayName`, `Icon`, `WorldMesh`, `MaxStackSize`, `ItemType`, `Category`.
3. Если это инструмент → `ToolType`, `EquipMesh`, `ToolDamage`.
4. Если расходник → `bIsConsumable=true`, `HungerRestore / ThirstRestore / HealthRestore`.
5. Если одежда → `Category=Cloth`, `ClothEquipMode`, `ClothSkeletalMesh` (с тем же скелетом!), `Protection`.
6. Создайте `BP_<Name>` на основе `AItemActor` в `Content/Items/`, в Defaults поставьте `ItemData=DA_<Name>`, при желании `PickupSound`.

### Я добавил **новый ресурс на сцене** (дерево, камень):

1. Создайте `DA_<Resource>` на основе `UResourceNodeData`: задайте `Mesh`, `RequiredTool`, `MaxHealth`, `DropItem`, `DropMin/Max`.
2. Поставьте `BP_ResourceNode` (или сделайте свой BP-наследник) на карту, в Defaults укажите `NodeData=DA_<Resource>` и `DropActorClass=BP_Item_<Что-выпадает>`.

### Я добавил **нового монстра**:

1. Сделайте скелетал-меш + Animation BP.
2. Создайте `BP_<Enemy>` на основе `AEnemyCharacter`.
3. Default Mesh, AnimBP — поставьте.
4. `BehaviorTree=BT_Ghost`. На контроллере (если меняли) → `Blackboard=BB_Ghost`.
5. На самом BP в Defaults:
   - `MaxHealth`, `AttackRange`, `AttackMontage`, `DeathLingerTime`.
   - `LootTable` — что выпадает. `DropActorClass` — какой BP_Item спавнить.
6. На вкладке *Components → AttackComponent*: `AttackDamage`, `AttackCapsuleRadius/HalfHeight/ForwardReach`, звуки.
7. В `AttackMontage` поставьте `AnimNotify_EnemyAttackHit` на кадре удара.
8. На сцене расставьте `Target Point` актёры → перетащите в `PatrolPoints` инстанса моба.

### Я добавил **нового червя**:

1. Скелетал-меш + AnimBP.
2. `BP_<Worm>` на основе `AWormEnemy`.
3. Назначьте `MeshComponent.SkeletalMesh + AnimBlueprint`.
4. Подгоните `DetectionTrigger` и `KillCapsule`.
5. Defaults: `TelegraphDelay`, `AttackDuration`, `RiseDistance`, `RiseDuration`, `BurrowDuration`, `StrikeDamage`.
6. `AttackMontage`, `RiseMontage` (для цикла подъёма).
7. `MaxHealth`, `DeathLingerTime`.
8. `LootTable`, `DropActorClass`.
9. `CameraShakeClass`, звуки.

### Я добавил **новый рецепт**:

1. *Create → Miscellaneous → Data Asset → RecipeDataAsset* в `Content/Items/Recipes/`.
2. Задайте `DisplayName`, `RecipeIcon`, `ResultItem`, `ResultQuantity`, заполните `Ingredients`.
3. Откройте `WBP_Crafting` → в Defaults добавьте новый рецепт в `AvailableRecipes`.

### Я хочу **добавить новый Input action**:

1. Создайте `IA_<Name>` в `Content/Input/Protagonist/`.
2. Привяжите в `IMC_Default` к клавише.
3. В C++ добавьте `UPROPERTY(EditDefaultsOnly) TObjectPtr<UInputAction> <Name>Action;` и `BindAction` в `SetupPlayerInputComponent` (без C++ нельзя — Enhanced Input биндится по типизированным указателям).
4. На `BP_ProtagonistCharacter` в Defaults укажите ассет.

### Я хочу **глобально замедлить голод/жажду**:

1. На игроке → `AttributeComponent → HungerDrainPerSecond / ThirstDrainPerSecond` (базовая скорость).
2. На `ADayNightCycleManager → DayThirstMultiplier / NightHungerMultiplier` (множители фазы).

### Я хочу **сделать день длиннее**:

1. `ADayNightCycleManager → DayDurationSeconds / NightDurationSeconds`.

---

## Куда смотреть, если что-то не работает

| Симптом | Скорее всего |
|---|---|
| Удар по дереву ничего не делает | `RequiredTool` в `DA_<Resource>` не совпадает с `ToolType` экипированного предмета. |
| Ресурс/моб умер, но дроп не появился | Не назначен `DropActorClass` в BP. |
| Игрок не атакует | В активном слоте хотбара нет предмета с `ToolType != None`. |
| Враг видит сквозь стену | На AI Perception → Sight Config «Detect by Affiliation»/«AutoSuccessRange» не должны позволять обнаружение через стены. Проверьте `SightConfig` в `AEnemyAIController` (или его BP-обёртке). |
| Враг не выходит из боя | На узлах BT, проверяющих `TargetActor`, нет *Observer Aborts: Both*. |
| Червь не наносит урона | `KillCapsule` слишком маленькая или неправильно расположена. Включите `bDebugDrawKillCapsule`. |
| Камера ныряет внутрь игрока | `CameraBoom` — обычный `USpringArmComponent` вместо `UPlayerSpringArmComponent`. |
| Меш инструмента болтается у ног | Скелет не имеет сокета `hand_r_socket`. Поменяйте `ToolAttachSocket` в `BP_ProtagonistCharacter`. |
| Плащ не двигается с телом | `ClothSkeletalMesh` использует **другой** скелет, чем тело игрока. |
| Подобранный предмет возрождается после загрузки | `RegisterPickedUpItem` не вызывается (стандартный `AItemActor` это делает сам — не переопределяйте `TryPickup` без вызова `Super`). |
