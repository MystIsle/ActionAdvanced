# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

ActionAdvanced — UE 5.7 advanced action/combat feel demo. Built from the Third Person template, it has **two first-party C++ modules** — the `ActionAdvanced` game module (`Source/`) and an in-repo **`ActionCore`** runtime plugin (`Plugins/ActionCore/`) that holds the reusable, GameplayTag-driven action system — plus a **vendored `MeleeTrace` plugin** (`Plugins/MeleeTrace/`, a git **submodule** of `rlewicki/MeleeTrace`) used for swept melee hit detection. Engine version is pinned to 5.7 (`ActionAdvanced.uproject`, `EngineIncludeOrderVersion.Unreal5_7`).

Plugins enabled in `.uproject`: `StateTree`, `GameplayStateTree`, `ModelingToolsEditorMode` (editor only), `MotionWarping`, and `MeleeTrace`. The bundled `ActionCore` plugin is *not* listed in `.uproject` — it sits under `Plugins/` with `"EnabledByDefault": true`, so it auto-enables (MeleeTrace, by contrast, is *not* EnabledByDefault, so it's listed explicitly). Game module public dependencies: `EnhancedInput`, `AIModule`, `StateTreeModule`, `GameplayStateTreeModule`, `UMG`, `Slate`, `MotionWarping`, `ActionCore`, and `MeleeTrace` (see `Source/ActionAdvanced/ActionAdvanced.Build.cs`). `ActionCore` depends on `GameplayTags` (public) plus `MotionWarping` + `AIModule` (private). **MeleeTrace is a git submodule** — a fresh clone needs `git submodule update --init` or it'll be an empty folder and the build will fail.

## Tooling — prefer Rider MCP

This project is worked on through JetBrains Rider, and the Rider MCP server is connected. **Prefer the Rider MCP tools over generic shell/file tools** whenever an equivalent exists — they use the IDE's index and understand the C++/UBT solution, so they're faster and more accurate than `cat`/`grep`/`find`:

- Read files with `get_file_text_by_path` (pass `projectPath` = the repo root to avoid ambiguity), not `Read`/`cat`.
- Search code with `search_in_files_by_text` / `search_in_files_by_regex`, not `grep`.
- Find files with `find_files_by_glob` / `find_files_by_name_keyword`, not `find`.
- Browse structure with `list_directory_tree`, not `ls`.
- For symbols, refactors, problems, and builds, use the symbol/refactor/`get_file_problems`/`build_solution` tools rather than ad-hoc shell.

Fall back to the built-in Bash/Read/Edit tools only for things Rider MCP doesn't cover (e.g. `git`, UBT CLI builds on macOS, file edits).

**Symbol vs text boundary (the key distinction):** use Rider for *symbol* work — finding where a symbol is defined/used, renames, reference updates (`search_symbol`, `rename_refactoring`) — because `Grep` is plain text matching and can't tell a definition from a comment. Plain *text/log/comment/string* pattern search is fine with the built-in `Grep`; simple path-glob file finding (`*.cpp`) is fine with `Glob`; single text replacements are fine with `Edit`. If the Rider IDE is closed and an MCP call fails, fall back to the built-in tool. A global PreToolUse hook (`~/.claude/hooks/rider-nudge.js`) auto-reminds on `Grep`/`Edit`/`Glob` (warn-only, once per tool per session).

## Build / run / generate

Project files and builds go through Unreal Build Tool (UBT) — there is no separate package manager. This repo is developed on **both Windows and macOS** (the project targets Windows DX12 SM6, Mac Metal SM6, Linux Vulkan SM6 per `Config/DefaultEngine.ini`). Substitute `UE_ROOT` for your engine install (Windows: `C:\Program Files\Epic Games\UE_5.7`; macOS: `/Users/Shared/Epic Games/UE_5.7`).

```bash
# Regenerate IDE project files (after adding/removing .cpp/.h or editing *.Build.cs)
"$UE_ROOT/Engine/Build/BatchFiles/Mac/GenerateProjectFiles.sh" -project="$(pwd)/ActionAdvanced.uproject" -game -rocket

# Compile the editor target (what you run when iterating in-editor)
"$UE_ROOT/Engine/Build/BatchFiles/Mac/Build.sh" ActionAdvancedEditor Mac Development -project="$(pwd)/ActionAdvanced.uproject"

# Compile the standalone game target
"$UE_ROOT/Engine/Build/BatchFiles/Mac/Build.sh" ActionAdvanced Mac Development -project="$(pwd)/ActionAdvanced.uproject"

# Launch the editor on this project
"$UE_ROOT/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" "$(pwd)/ActionAdvanced.uproject"
```

On Windows (current dev host), the equivalents:

```powershell
# Regenerate IDE project files (after adding/removing .cpp/.h, editing *.Build.cs, or adding a plugin)
& "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\GenerateProjectFiles.bat" -project="C:\GitRepositories\ActionAdvanced\ActionAdvanced.uproject" -game -rocket

# Compile the editor target — this doubles as the build-verify command
& "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" ActionAdvancedEditor Win64 Development -project="C:\GitRepositories\ActionAdvanced\ActionAdvanced.uproject" -waitmutex
```

Targets are defined by `Source/ActionAdvanced.Target.cs` (Game) and `Source/ActionAdvancedEditor.Target.cs` (Editor).

**Build verification (Windows) — important:** trust the **UBT CLI**, not Rider's MCP `build_solution`. The MCP tool *always* returns `{isSuccess:false}` for this UE solution even when the build succeeds (it does run a real UBT build under the hood — it just misreports). Read the real outcome from `%LOCALAPPDATA%\UnrealBuildTool\Log.txt` (first line `Log started at <time>` to confirm freshness; final `Result: Succeeded`/`Result: Failed` for the verdict; `error:` lines on failure). Rider's IDE Build Solution (Ctrl+Shift+B) works fine — only the MCP bridge misreports. Rider `get_file_problems` also emits false `';'` errors on UE reflection macros (`UCLASS`/`UENUM`/`UPROPERTY`/`GENERATED_BODY`) — if the same error hits pre-existing code too, it's a false positive; ignore it. Editing only `.uplugin`/`.Build.cs` may leave UBT saying "Target is up to date"; touch `ActionAdvanced.uproject`'s mtime (content unchanged) to force a makefile regen.

There is no automated test suite wired in. To run any Automation/Functional tests, use the in-editor `Session Frontend → Automation` panel, or via CLI:

```bash
"$UE_ROOT/Engine/Binaries/Mac/UnrealEditor-Cmd" "$(pwd)/ActionAdvanced.uproject" \
  -ExecCmds="Automation RunTests <TestFilter>; Quit" -unattended -nopause -testexit="Automation Test Queue Empty"
```

## Source layout — important nuance

The C++ module lives at `Source/ActionAdvanced/`, and inside it there is a **second** `ActionAdvanced/` subdirectory that holds the *active* in-development code:

- `Source/ActionAdvanced/ActionAdvanced/Core/` — `AAGameMode` (new game mode shell, currently empty)
- `Source/ActionAdvanced/ActionAdvanced/Characters/` — `AACharacter`, `AAPlayerController`, `AAMonsterController`. These are the canonical character / controller classes being built out. `AAACharacter` overrides the movement component with the plugin's `UACCharacterMovementComponent` (via `SetDefaultSubobjectClass` in its `FObjectInitializer` ctor) and owns a `UACActionComponent` (from the ActionCore plugin). Enhanced Input bindings (Move / Look / Jump / Sprint / LightAttack / HeavyAttack) live on `AAAPlayerController`. The controller caches the pawn's `UACActionComponent` in `OnPossess` (`CachedActionComponent`) and routes attacks through its own `PlayAction(FGameplayTag)` helper: **the two Attack handlers are now wired** (`OnInputLightAttackStarted` → `PlayAction(LightAttackTag)`, `OnInputHeavyAttackStarted` → `PlayAction(HeavyAttackTag)`, with `LightAttackTag`/`HeavyAttackTag` exposed as `EditDefaultsOnly` properties). **Only `OnInputSprint` is still a TODO stub** that logs — wiring it to a `MaxWalkSpeed` toggle on `AACharacter` is the next step.
- `Source/ActionAdvanced/Legacy/` — the original template's `ActionAdvancedCharacter` / `ActionAdvancedPlayerController` / `ActionAdvancedGameMode`. Kept around because `DefaultEngine.ini` still has class redirects from `TP_ThirdPerson*` to `ActionAdvanced*`, and the default game mode reference in BP (`/Game/ThirdPerson/Blueprints/BP_ThirdPersonGameMode`) still depends on them. Don't delete without auditing the redirects and BP refs.
- `Source/ActionAdvanced/Variant_Combat/`, `Variant_Platforming/`, `Variant_SideScrolling/` — full template variants kept as reference implementations. Each has its own `Character` / `PlayerController` / `GameMode` plus topic folders (`AI/`, `Animation/`, `Gameplay/`, `Interfaces/`, `UI/`). The Build.cs adds every one of these directories to `PublicIncludePaths`, so any header inside them is includable by bare name (e.g. `#include "CombatCharacter.h"`), which is something to remember when adding new code that might clash on filename.

Naming convention: new code uses the `AA` prefix (e.g. `AAACharacter`, `AAAPlayerController` — the leading `A` is UE's actor prefix on top of the `AA` class prefix). Legacy code keeps `ActionAdvanced` in the class name. Variant code is prefixed by its variant (e.g. `ACombatCharacter`, `ASideScrollingNPC`).

`Source/ActionAdvanced/ActionAdvanced.h` declares the project's log category: `LogActionAdvanced`. Use it for any project-level logging instead of `LogTemp`. The plugin has its own category `LogActionCore` (in `Plugins/ActionCore/Source/ActionCore/ActionCore.h` — module **root**, not `Public/`; see the Action system section) — use that for code inside the plugin.

## Action system (ActionCore plugin)

`Plugins/ActionCore/Source/ActionCore/` is a standalone Runtime module (`ACTIONCORE_API`, `AC` class prefix) implementing a GameplayTag-keyed, montage-driven action system built around a **cancel-window** model (actions interrupt each other only inside authored cancel windows, not the older "one action at a time" rule). The pieces, and the ownership chain between them:

- **`UACActionDataAsset`** (`UDataAsset`) — the authored definition: a `Key` (`FGameplayTag`) + an `AnimMontage`. Editor `IsDataValid` rejects an empty Key or Montage. Author one asset per action.
- **`UACActionComponent`** (`UActorComponent`, lives on the character) — holds an editable `TArray<UACActionDataAsset*> DataAssets`. In `InitializeComponent` it builds a `TMap<FGameplayTag, UACAction*> Actions` (skipping null assets, warning on duplicate keys). Entry point `PlayAction(FGameplayTag, FRotator)` (the `FRotator` is the desired facing, fed to the per-action rotation/lunge warp — see the combat layer) returns the live `UACActionInstance*`. **Cancel gating**: `CanPlayAction` returns true when there is no `PlayingInstance` **or** the current `PlayingInstance->IsCancelable()` — so a new action interrupts the running one as soon as it has entered its cancel window, not only after it fully ends. It tracks `PlayingInstance` (most-recent, used for gating) plus `ActivateInstances` — a `TMap<int32 MontageInstanceID, UACActionInstance*>` so branching-point notifies can resolve the live instance by montage-instance id (`GetActivateInstance`). It also owns the **reference-counted movement lock**: `SetMovementLocked(bool)` adjusts `MovementLockedCount` and, only on the 0↔1 edge, casts the owner's CMC to `UACCharacterMovementComponent` and calls `SetMovementInputLocked(...)` — a *pawn-local* CMC lock (enforced in `ConsumeInputVector`), deliberately **not** `Controller->SetIgnoreMoveInput`, which UnPossess doesn't reset and so leaked the lock onto a respawned pawn. (Open `TODO`: AI `RequestDirectMove`/`RequestedVelocity` paths bypass `ConsumeInputVector`, so this lock doesn't stop them yet.) `EndPlay` calls `Stop()` on every active instance so instance-tied effects unwind cleanly. `bWantsInitializeComponent = true`; ticking is off.
- **`UACCharacterMovementComponent`** (`UCharacterMovementComponent`, the plugin's base CMC — `AAACharacter` installs it via `SetDefaultSubobjectClass`, and the component's movement lock above only bites if the character's CMC derives from this) — two jobs: (1) the **movement-input lock** — `SetMovementInputLocked(bool)` flips a flag that the overridden `ConsumeInputVector` reads to zero out pending input (Super is still called so input is consumed, not accumulated); (2) **air-rotation damping** — caches a rotation scale once per movement-mode change (`OnMovementModeChanged` → `AirRotationScale` when falling, else 1.0) and multiplies `GetDeltaRotation` by it, avoiding a per-frame branch. The damping used to live in a game-module subclass `UAACharacterMovementComponent`; that subclass was folded into this base class and deleted, so the character now uses this type directly.
- **`UACAction`** (`UObject`) — the per-key runtime definition created from a DataAsset by the component. `Play()` checks `CanPlay()` (which delegates to `Component->CanPlayAction`), news up a fresh `UACActionInstance`, and returns it only if it reached `Ready` and its own `Play()` succeeded.
- **`UACActionInstance`** (`UObject`) — one live playback. `Play()` calls `Owner->PlayAnimMontage(...)`, then `CharacterMovementComponent->StopMovementImmediately()` and **locks movement**, caches the `FAnimMontageInstance`'s `MontageInstanceID`, binds `OnMontageBlendingOutStarted` / `OnMontageEnded`, and registers with the component. State machine: `None → Ready → Playing → BlendingOut → Finished` (`IsPlaying()` is true for Playing and BlendingOut; the terminal state is **`Finished`**, not "Stopped"). **Cancelability**: `MarkCancelable()` flips `bCancelable` *and unlocks movement* (this is what opens the cancel window); it is invoked (a) mid-montage by the branching-point notify below, and (b) automatically when a non-interrupted blend-out starts. `Stop()` → `StopInternal` interrupts the montage and unwinds the lock.
- **`FACActionContext`** (a plain `ACTIONCORE_API` struct in `ACActionContext.h`, **not** a `UObject`) — constructed from an `FBranchingPointNotifyPayload`; it resolves payload → owner `ACharacter` → its `UACActionComponent` → the live `UACActionInstance` (via `GetActivateInstance(MontageInstanceID)`). Holds weak ptrs; `IsValid()` gates use.
- **`UACAnimNotify_Cancelable`** (`UAnimNotify`, a **native branching point** — `bIsNativeBranchingPoint = true`) — the authoring hook for cancel windows. Placed on a montage's timeline, its `BranchingPointNotify` builds an `FACActionContext` and calls `Instance->MarkCancelable()`. Where you drop this notify on each montage *is* where that action becomes cancelable.

Flow: input → `AAAPlayerController` (its `CachedActionComponent`, resolved in `OnPossess`) → `PlayAction(Tag)` → component looks up the `UACAction` → `UACAction::Play()` → new `UACActionInstance` plays the montage, locks movement, and stays uncancelable until a `UACAnimNotify_Cancelable` branching point (or blend-out) opens its cancel window. Chaining a combo = press the next action's input during that window; `CanPlayAction` lets it through and the new instance interrupts the old one. Note `UACActionInstance::Play()` still carries a `TODO` about inertialization / blending feel, and `MarkCancelable` carries a `NOTE` that move-cancel and attack-cancel are not yet separated (it unlocks movement at the same moment it opens the attack cancel window) — the "action feel" tuning is expected to land in those spots.

Building out from this core: add a `UACActionDataAsset` per move, register it on the character's `ActionComponent.DataAssets`, set the controller's `LightAttackTag` / `HeavyAttackTag` (and add handlers/tags for new moves), and place `UACAnimNotify_Cancelable` notifies on each montage to tune its cancel window.

`ActionCore.{h,cpp}` (the module entry + the `LogActionCore` category) live at the **module root** (`Plugins/ActionCore/Source/ActionCore/`), *not* under `Public/`/`Private/`. This is intentional: `ActionCore.Build.cs` adds `PrivateIncludePaths.AddRange([ModuleDirectory])` so `#include "ActionCore.h"` resolves. Don't "tidy" these two files back into `Public/`/`Private/` or drop that line — the build breaks. `ActionCore.Build.cs` also uses C# collection-expression syntax (`[...]`) for every list — match that when editing it.

## Combat layer on top of the action system

Built on the ActionCore action system + the `MeleeTrace` plugin, wired through the `AA` character/controllers. This spans several files; the big picture:

- **Teams** (`IGenericTeamAgentInterface`) — `EAATeamID` (`Player`/`Monster`/`NoTeam`) lives in `Source/ActionAdvanced/ActionAdvanced/Team/AATeamID.h` (game module — teams are game-specific). `AAACharacter` is the single source of truth (`TeamID` `EditDefaultsOnly` property + inline `GetGenericTeamId()`); `AAAPlayerController` / `AAAMonsterController` implement the interface and **delegate to their possessed pawn**. Attitude uses the engine default solver (same team = Friendly, else Hostile). **`ActionCore` consumes teams only through the engine interface (`FGenericTeamId::GetAttitude`) and never references `EAATeamID`** — that one-way dependency (game module → plugin, never back) is deliberate, keeping the plugin game-agnostic. To make the cross-folder include `"Team/AATeamID.h"` resolve, `ActionAdvanced/ActionAdvanced` was added to `PublicIncludePaths` (the active-code subdir wasn't an include root before).
- **Per-action facing & lunge** (`UACActionDataAsset`) — `RotationDirection` (`EACActionDirection` `None`/`Input`/`Control`) chooses the rotation the action warps to face; `bAutoTargetLunge` + `LungeRange` / `LungeHalfAngle` / `LungeOffset` make the action rush to a stand-off point (caster radius + target radius + offset) in front of the best hostile target. Resolved in `UACActionInstance::Play` via `DetermineFacingRotation` / `TryLungeWarp`, applied through the same `"Target"` motion-warp target the rotation feature uses. Air actions normally disable root motion; a lunge keeps it on so the warp still pulls the character in mid-air.
- **Auto-targeting** (`UACTargetingLibrary::FindBestTarget`, a stateless `UBlueprintFunctionLibrary`) — per-skill, queried **once at fire time** (no tick/subsystem — deliberately simplified to a library): sphere-overlap on `ECC_Pawn`, filter to Hostile via the team interface, score by weighted distance + angular alignment, return the best within the action's cone. CVar `AA.AutoTarget.Debug 1` draws the cone / candidates / pick.
- **Melee hit detection** (`MeleeTrace` plugin) — `UMeleeTraceComponent` on `AAACharacter`; attack montages carry **`UACAnimNotifyState_MeleeHit`** (ActionCore's subclass of the plugin's `UAnimNotifyState_MeleeTrace`, also a native branching point). Its `BranchingPointNotifyBegin`/`End` chain to `Super` (so the plugin's swept-sphere trace still runs) *and* open/close the live action's detection window via `UACActionInstance::BeginHitDetection(FACHitEffect)` / `EndHitDetection`, which subscribe to `MeleeTraceComponent->OnTraceHit` **only for the notify window**. **The hit seam moved off the character onto the action instance** — `UACActionInstance::OnMeleeTraceHit` is now the handler (the old `AAACharacter::OnMeleeTraceHit` `BeginPlay` binding was removed). The plugin only ignores the owner, so **team filtering is still the consumer's job**: the handler drops non-Hostile hits via `FGenericTeamId::GetAttitude`. Trace channel (Pawn vs PhysicsBody vs a custom Melee channel) is still being decided.
- **Hitstun & knockback ("경직/넉백")** — the old TODO, now implemented. Each melee notify authors an **`FACHitEffect`** (`ACHitEffect.h`, a `BlueprintType` struct): hitstop (`HitStopDuration` + `HitStopPlayRate`, applied as the skeletal mesh's `GlobalAnimRateScale`) and knockback (`KnockbackDistance` / `KnockbackDuration` / optional `KnockbackEaseCurve`). On a confirmed hostile hit, `OnMeleeTraceHit` calls the **victim's** `UACHitReactionComponent::PlayReact(Effect, Direction)` and gives the **attacker** a self `RequestHitStop`. `UACHitReactionComponent` is a **4th component on `AAACharacter`** (alongside Action / MotionWarping / MeleeTrace) and drives the reaction: stop the in-progress action, lock movement (reusing the CMC movement lock) and pause the AI `BrainComponent`, snap-face the attacker, play a cycled hit montage (`HitReactMontages`), run the hitstop, then knockback via `FRootMotionSource_MoveToDynamicForce` (eased by the curve). Re-hit and `EndPlay` unwind through `CancelReact` so the movement lock, root-motion source, and `GlobalAnimRateScale` never leak onto a reused CMC/mesh. Knockback ease curves are authored as `CF_*Knockback` `UCurveFloat` assets; reaction montages as `AM_Hit_*`.

Korean design/planning docs for these features live in `Docs/` (e.g. `경직-넉백-설계`, `오토타게팅-설계`, `히트-노티파이-설계`, `팀설정-IGenericTeamInterface-이식-설계`, `액션-회전방향-옵션화-설계`, plus the overall `특강-액션고도화-기획`). When extending one of these features, read its design doc first — it captures the intent and the deliberately-cut scope. (`Docs/` may still be untracked locally — `git status` to check before relying on it being committed.)

## Content & maps

- Default map: `/Game/ActionAdvanced/Maps/L_Demo` (set as both `GameDefaultMap` and `EditorStartupMap`).
- `GlobalDefaultGameMode` is still the Blueprint `BP_ThirdPersonGameMode` from the template — when wiring up `AAAGameMode`, you'll also need to set the level's game mode override or repoint the global default.
- Content/`Quaternius/` — 3D assets credited under CC0; treat as imported reference content.
- Licensing (per README/LICENSE): the project's **original code is MIT-licensed**; Unreal Engine code and default content stay under the Epic Games EULA. The repo is meant to be published IP-clean — don't vendor in license-incompatible code or paid-asset content.
- Content is tracked via Git LFS (see `.gitattributes` — `.uasset`, `.umap`, plus all common image/audio/video formats).
- `Saved/`, `Intermediate/`, `DerivedDataCache/`, `Binaries/`, `.idea` are gitignored — never commit them.

## Renderer / project settings worth knowing

`Config/DefaultEngine.ini` is tuned as a **deliberately lightweight, Mac/Apple-Silicon-friendly profile** — Lumen, ray tracing, Nanite, and Virtual Shadow Maps are all **off**:
- **GI & Reflections are Screen Space, NOT Lumen.** `r.DynamicGlobalIlluminationMethod=2` and `r.ReflectionMethod=2` both select **Screen Space** — the `EDynamicGlobalIlluminationMethod` / `EReflectionMethod` enums (`Engine/Classes/Engine/EngineTypes.h`) are `0=None, 1=Lumen, 2=ScreenSpace`. So **Lumen is off** (value `2` is *not* Lumen — easy to misread). The `r.Lumen.TraceMeshSDFs=0` line is inert while Lumen isn't the active GI method.
- `r.AllowStaticLighting=False` — no baked GI either, so GI is screen-space-only with no fallback. Setting the GI method to `0` (None) would leave direct lighting only.
- **Ray Tracing off** (`r.RayTracing=False`); the `r.RayTracing.RayTracingProxies.ProjectEnabled=True` line is inert while the RT master is off.
- **Nanite disabled** project-wide (`r.Nanite.ProjectEnabled=False`) — don't assume Nanite when authoring assets.
- **Virtual Shadow Maps off** (`r.Shadow.Virtual.Enable=0`) → conventional shadow maps. VSM pairs with Nanite (which is off), so this is intentional.
- Substrate enabled (`r.Substrate=True`); mesh distance fields still generated (`r.GenerateMeshDistanceFields=True`).
- Targeted RHIs: DX12 SM6 on Windows, Metal SM6 on Mac, Vulkan SM6 on Linux.

These choices are deliberate: commit `fdbcb97` ("나나이트 루멘 비활성화") switched GI/reflections off Lumen to Screen Space and disabled Nanite; RT and VSM were turned off afterward on `feature/action_core` to keep the editor light on Apple Silicon. Think twice before flipping them back.

## Scope discipline — this repo is a deliberately minimal demo

Make only the requested change, surgically. Do not introduce unrequested abstractions — no interface extraction, no new subsystems/managers, no event buses, no "for extensibility" refactors. Do not "improve" the deliberate simplifications: auto-targeting is intentionally a stateless library (not a tick/subsystem), the action system is intentionally non-GAS, and multiplayer is intentionally out of scope. `Legacy/` and `Variant_*` are frozen reference code. `TODO`/`NOTE` comments in the code are planned next steps — don't implement them preemptively. When in doubt, ask.

## Repo conventions

- Commit messages and branch names in this repo are written in Korean. Match the existing style when adding new commits unless the user asks otherwise (e.g. `액션 시스템 코어 : 구현`).
- Feature work happens on `feature/*` branches (e.g. `feature/action_core`), which merge into `develop`; `develop` integrates toward `main` via `release/*` milestone branches (e.g. `release/lecture01`, already merged to `main`). PRs target `main`. (Currently working on `lecture02`, branched from `main`, for the lecture part-2 work.)
- The build artifacts under `Plugins/ActionCore/Intermediate/` and `Plugins/ActionCore/Binaries/` are generated — don't commit them (same rule as the top-level `Intermediate/`, `Binaries/`).
