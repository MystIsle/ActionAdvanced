# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

ActionAdvanced — UE 5.7 advanced action/combat feel demo. Single C++ game module (`ActionAdvanced`) built from the Third Person template. Engine version is pinned to 5.7 (`ActionAdvanced.uproject`, `EngineIncludeOrderVersion.Unreal5_7`).

Required engine plugins (already enabled in `.uproject`): `StateTree`, `GameplayStateTree`, `ModelingToolsEditorMode` (editor only). Public module dependencies include `EnhancedInput`, `AIModule`, `StateTreeModule`, `GameplayStateTreeModule`, `UMG`, `Slate` (see `Source/ActionAdvanced/ActionAdvanced.Build.cs`).

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
- `Source/ActionAdvanced/ActionAdvanced/Characters/` — `AACharacter`, `AAPlayerController`, `AAMonsterController`. These are the canonical character / controller classes being built out (Enhanced Input bindings: Move / Look / Jump / Sprint / LightAttack / HeavyAttack are declared on the player controller).
- `Source/ActionAdvanced/Legacy/` — the original template's `ActionAdvancedCharacter` / `ActionAdvancedPlayerController` / `ActionAdvancedGameMode`. Kept around because `DefaultEngine.ini` still has class redirects from `TP_ThirdPerson*` to `ActionAdvanced*`, and the default game mode reference in BP (`/Game/ThirdPerson/Blueprints/BP_ThirdPersonGameMode`) still depends on them. Don't delete without auditing the redirects and BP refs.
- `Source/ActionAdvanced/Variant_Combat/`, `Variant_Platforming/`, `Variant_SideScrolling/` — full template variants kept as reference implementations. Each has its own `Character` / `PlayerController` / `GameMode` plus topic folders (`AI/`, `Animation/`, `Gameplay/`, `Interfaces/`, `UI/`). The Build.cs adds every one of these directories to `PublicIncludePaths`, so any header inside them is includable by bare name (e.g. `#include "CombatCharacter.h"`), which is something to remember when adding new code that might clash on filename.

Naming convention: new code uses the `AA` prefix (e.g. `AAACharacter`, `AAAPlayerController` — the leading `A` is UE's actor prefix on top of the `AA` class prefix). Legacy code keeps `ActionAdvanced` in the class name. Variant code is prefixed by its variant (e.g. `ACombatCharacter`, `ASideScrollingNPC`).

`Source/ActionAdvanced/ActionAdvanced.h` declares the project's log category: `LogActionAdvanced`. Use it for any project-level logging instead of `LogTemp`.

## Content & maps

- Default map: `/Game/ActionAdvanced/Maps/L_Demo` (set as both `GameDefaultMap` and `EditorStartupMap`).
- `GlobalDefaultGameMode` is still the Blueprint `BP_ThirdPersonGameMode` from the template — when wiring up `AAAGameMode`, you'll also need to set the level's game mode override or repoint the global default.
- Content/`Quaternius/` — 3D assets credited under CC0; treat as imported reference content.
- Content is tracked via Git LFS (see `.gitattributes` — `.uasset`, `.umap`, plus all common image/audio/video formats).
- `Saved/`, `Intermediate/`, `DerivedDataCache/`, `Binaries/`, `.idea` are gitignored — never commit them.

## Renderer / project settings worth knowing

`Config/DefaultEngine.ini`:
- Lumen GI enabled (`r.DynamicGlobalIlluminationMethod=2`) **but** mesh SDF traces are disabled (`r.Lumen.TraceMeshSDFs=0`).
- Nanite is **disabled** at the project level (`r.Nanite.ProjectEnabled=False`). Don't assume Nanite when authoring assets.
- Substrate enabled (`r.Substrate=True`).
- Ray Tracing on; static lighting off (`r.AllowStaticLighting=False`).
- Targeted RHIs: DX12 SM6 on Windows, Metal SM6 on Mac, Vulkan SM6 on Linux.

These choices are deliberate (commit `fdbcb97` — "나나이트 루멘 비활성화" toggled them); think twice before flipping them back.

## Repo conventions

- Commits/branch in this repo are written in Korean. Match the existing style when adding new commits unless the user asks otherwise.
- Working branch is `develop`; PRs target `main`.
