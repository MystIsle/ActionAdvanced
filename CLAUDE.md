# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

ActionAdvanced — UE 5.7 advanced action/combat feel demo. Built from the Third Person template, it now has **two** C++ modules: the `ActionAdvanced` game module (`Source/`) and an in-repo **`ActionCore`** runtime plugin (`Plugins/ActionCore/`) that holds the reusable, GameplayTag-driven action system. Engine version is pinned to 5.7 (`ActionAdvanced.uproject`, `EngineIncludeOrderVersion.Unreal5_7`).

Required engine plugins (already enabled in `.uproject`): `StateTree`, `GameplayStateTree`, `ModelingToolsEditorMode` (editor only). The bundled `ActionCore` plugin is *not* listed in `.uproject` — it sits under `Plugins/` with `"EnabledByDefault": true`, so it auto-enables. Game module public dependencies: `EnhancedInput`, `AIModule`, `StateTreeModule`, `GameplayStateTreeModule`, `UMG`, `Slate`, and `ActionCore` (see `Source/ActionAdvanced/ActionAdvanced.Build.cs`). `ActionCore` itself depends on `GameplayTags`.

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

Project files and builds go through Unreal Build Tool (UBT) — there is no separate package manager. The host here is macOS (`darwin`), but the project also targets Windows (DX12, SM6) and Linux/Mac (Vulkan/Metal SM6) per `Config/DefaultEngine.ini`. Substitute `UE_ROOT` for your engine install (e.g. `/Users/Shared/Epic Games/UE_5.7`).

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

On Windows substitute the corresponding `Engine/Build/BatchFiles/Build.bat` / `GenerateProjectFiles.bat`. Targets are defined by `Source/ActionAdvanced.Target.cs` (Game) and `Source/ActionAdvancedEditor.Target.cs` (Editor).

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
- **`UACActionComponent`** (`UActorComponent`, lives on the character) — holds an editable `TArray<UACActionDataAsset*> DataAssets`. In `InitializeComponent` it builds a `TMap<FGameplayTag, UACAction*> Actions` (skipping null assets, warning on duplicate keys). Entry point `PlayAction(FGameplayTag)` returns the live `UACActionInstance*`. **Cancel gating**: `CanPlayAction` returns true when there is no `PlayingInstance` **or** the current `PlayingInstance->IsCancelable()` — so a new action interrupts the running one as soon as it has entered its cancel window, not only after it fully ends. It tracks `PlayingInstance` (most-recent, used for gating) plus `ActivateInstances` — a `TMap<int32 MontageInstanceID, UACActionInstance*>` so branching-point notifies can resolve the live instance by montage-instance id (`GetActivateInstance`). It also owns the **reference-counted movement lock**: `SetMovementLocked(bool)` adjusts `MovementLockedCount` and, only on the 0↔1 edge, casts the owner's CMC to `UACCharacterMovementComponent` and calls `SetMovementInputLocked(...)` — a *pawn-local* CMC lock (enforced in `ConsumeInputVector`), deliberately **not** `Controller->SetIgnoreMoveInput`, which UnPossess doesn't reset and so leaked the lock onto a respawned pawn. (Open `TODO`: AI `RequestDirectMove`/`RequestedVelocity` paths bypass `ConsumeInputVector`, so this lock doesn't stop them yet.) `EndPlay` calls `Stop()` on every active instance so instance-tied effects unwind cleanly. `bWantsInitializeComponent = true`; ticking is off.
- **`UACCharacterMovementComponent`** (`UCharacterMovementComponent`, the plugin's base CMC — `AAACharacter` installs it via `SetDefaultSubobjectClass`, and the component's movement lock above only bites if the character's CMC derives from this) — two jobs: (1) the **movement-input lock** — `SetMovementInputLocked(bool)` flips a flag that the overridden `ConsumeInputVector` reads to zero out pending input (Super is still called so input is consumed, not accumulated); (2) **air-rotation damping** — caches a rotation scale once per movement-mode change (`OnMovementModeChanged` → `AirRotationScale` when falling, else 1.0) and multiplies `GetDeltaRotation` by it, avoiding a per-frame branch. The damping used to live in a game-module subclass `UAACharacterMovementComponent`; that subclass was folded into this base class and deleted, so the character now uses this type directly.
- **`UACAction`** (`UObject`) — the per-key runtime definition created from a DataAsset by the component. `Play()` checks `CanPlay()` (which delegates to `Component->CanPlayAction`), news up a fresh `UACActionInstance`, and returns it only if it reached `Ready` and its own `Play()` succeeded.
- **`UACActionInstance`** (`UObject`) — one live playback. `Play()` calls `Owner->PlayAnimMontage(...)`, then `CharacterMovementComponent->StopMovementImmediately()` and **locks movement**, caches the `FAnimMontageInstance`'s `MontageInstanceID`, binds `OnMontageBlendingOutStarted` / `OnMontageEnded`, and registers with the component. State machine: `None → Ready → Playing → BlendingOut → Finished` (`IsPlaying()` is true for Playing and BlendingOut; the terminal state is **`Finished`**, not "Stopped"). **Cancelability**: `MarkCancelable()` flips `bCancelable` *and unlocks movement* (this is what opens the cancel window); it is invoked (a) mid-montage by the branching-point notify below, and (b) automatically when a non-interrupted blend-out starts. `Stop()` → `StopInternal` interrupts the montage and unwinds the lock.
- **`FACActionContext`** (a plain `ACTIONCORE_API` struct in `ACActionContext.h`, **not** a `UObject`) — constructed from an `FBranchingPointNotifyPayload`; it resolves payload → owner `ACharacter` → its `UACActionComponent` → the live `UACActionInstance` (via `GetActivateInstance(MontageInstanceID)`). Holds weak ptrs; `IsValid()` gates use.
- **`UACAnimNotify_Cancelable`** (`UAnimNotify`, a **native branching point** — `bIsNativeBranchingPoint = true`) — the authoring hook for cancel windows. Placed on a montage's timeline, its `BranchingPointNotify` builds an `FACActionContext` and calls `Instance->MarkCancelable()`. Where you drop this notify on each montage *is* where that action becomes cancelable.

Flow: input → `AAAPlayerController` (its `CachedActionComponent`, resolved in `OnPossess`) → `PlayAction(Tag)` → component looks up the `UACAction` → `UACAction::Play()` → new `UACActionInstance` plays the montage, locks movement, and stays uncancelable until a `UACAnimNotify_Cancelable` branching point (or blend-out) opens its cancel window. Chaining a combo = press the next action's input during that window; `CanPlayAction` lets it through and the new instance interrupts the old one. Note `UACActionInstance::Play()` still carries a `TODO` about inertialization / blending feel, and `MarkCancelable` carries a `NOTE` that move-cancel and attack-cancel are not yet separated (it unlocks movement at the same moment it opens the attack cancel window) — the "action feel" tuning is expected to land in those spots.

Building out from this core: add a `UACActionDataAsset` per move, register it on the character's `ActionComponent.DataAssets`, set the controller's `LightAttackTag` / `HeavyAttackTag` (and add handlers/tags for new moves), and place `UACAnimNotify_Cancelable` notifies on each montage to tune its cancel window.

`ActionCore.{h,cpp}` (the module entry + the `LogActionCore` category) live at the **module root** (`Plugins/ActionCore/Source/ActionCore/`), *not* under `Public/`/`Private/`. This is intentional: `ActionCore.Build.cs` adds `PrivateIncludePaths.AddRange([ModuleDirectory])` so `#include "ActionCore.h"` resolves. Don't "tidy" these two files back into `Public/`/`Private/` or drop that line — the build breaks. `ActionCore.Build.cs` also uses C# collection-expression syntax (`[...]`) for every list — match that when editing it.

## Content & maps

- Default map: `/Game/ActionAdvanced/Maps/L_Demo` (set as both `GameDefaultMap` and `EditorStartupMap`).
- `GlobalDefaultGameMode` is still the Blueprint `BP_ThirdPersonGameMode` from the template — when wiring up `AAAGameMode`, you'll also need to set the level's game mode override or repoint the global default.
- Content/`Quaternius/` — 3D assets credited under CC0; treat as imported reference content.
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

## Repo conventions

- Commit messages and branch names in this repo are written in Korean. Match the existing style when adding new commits unless the user asks otherwise (e.g. `액션 시스템 코어 : 구현`).
- Feature work happens on `feature/*` branches (current: `feature/action_core`), which merge into `develop`; `develop` integrates toward `main`. PRs target `main`.
- The build artifacts under `Plugins/ActionCore/Intermediate/` and `Plugins/ActionCore/Binaries/` are generated — don't commit them (same rule as the top-level `Intermediate/`, `Binaries/`).
