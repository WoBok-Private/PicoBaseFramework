# Pico Base Framework

**English** | [中文](README.zh-CN.md)

A reusable PICO VR player framework for Unreal Engine 5.4.4. It provides PicoBaseGameMode, PicoBasePawn, PicoBasePlayerController and PicoBaseStart, with desktop controls, headset/controller tracking and horizontal locomotion.

## Installation and scene setup

1. Install PICOXR 3.2.3 in the project and enable PICOXR, XRBase and EnhancedInput.
2. Copy the PicoBaseFramework folder into the project's Plugins directory, enable the plugin and compile the C++ project.
3. In World Settings, set GameMode Override to **PicoBaseGameMode**.
4. In Place Actors, search for **PicoBaseStart** and place its origin on the floor. Point its arrow in the initial viewing direction.

PicoBaseGameMode creates and possesses PicoBasePawn. The plugin creates its Enhanced Input mappings on demand and adds them to the local player when the pawn input is initialized. The framework uses PICOXR analog keys PICOTouch_Right_Thumbstick_X/Y and does not require OpenXR.

## PicoBaseStart settings

| Property | Default | Behavior |
| --- | --- | --- |
| Move Speed | 300 cm/s (3 m/s) | Desktop and right-stick movement speed. |
| Desktop Eye Height | 165 cm | Camera height above the floor origin in desktop mode. |
| Right Stick Dead Zone | 0.2 | VR stick dead zone; input beyond it is rescaled. |
| Enable Collision | Off | Block virtual locomotion with the player's collision capsule and slide along walls. |
| Priority | 0 | Higher values take precedence when multiple PicoBaseStart instances exist. |

PicoBasePawn also exposes **Enable Collision** under **Pico Base Framework**. When PicoBaseStart is selected for spawning, its setting is copied to the spawned pawn. Without PicoBaseStart, the pawn uses its class defaults.

The switch takes effect immediately when changed in Blueprint or in the editor's Details panel. C++ callers use **SetMovementCollisionEnabled**; **IsMovementCollisionEnabled** reads the current state.

## Spawn location and floor height

These rules apply to maps using PicoBaseGameMode to spawn the player.

| Available start | Spawn behavior |
| --- | --- |
| PicoBaseStart | Use its position and horizontal facing direction, taking precedence over Player Start. |
| Player Start only | Use the Player Start selected by UE; treat its origin Z as floor height. |
| Neither start | UE falls back to the World Settings position, normally world origin (0, 0, 0). |
| Play From Here without PicoBaseStart | Use the editor-provided spawn position and treat its Z as floor height. |

An explicit PicoBaseStart also takes precedence over Play From Here. Its XY coordinates define the virtual spawn position, its Z coordinate defines the virtual floor, and its arrow defines initial horizontal facing. Pitch, roll and scale do not alter the tracking coordinate system.

Usually place one PicoBaseStart per map. With multiple instances, the highest Priority wins; equal priorities are resolved by object path, with a warning in the log.

Place the origin of a regular Player Start on the floor as well. An elevated origin raises the player's virtual floor and camera.

Desktop camera height is Desktop Eye Height above the floor origin. VR camera height comes from tracked headset height. UE scene units are centimeters.

## Controls and movement

- Desktop: WASD movement; mouse up/down looks up/down, mouse right/left turns right/left.
- VR: tracked headset and controllers; right stick up/down moves forward/backward, right/left strafes, following the headset's horizontal facing.
- Mouse Sensitivity is exposed on PicoBasePawn and defaults to 1.
- Diagonal movement is normalized; VR stick input retains analog strength.
- Virtual movement preserves the player's floor Z coordinate.

With **Enable Collision** off, virtual movement passes through obstacles. With it on, the default movement capsule has a 25 cm radius and a 90 cm half-height, positioned 2 cm above the floor. It uses the **Pawn** collision profile: obstacles must block that channel. Movement sweeps prevent crossing blocking geometry and allow horizontal sliding along walls. Place the spawn capsule clear of blocking geometry.

Collision applies to desktop and joystick locomotion. Physical headset tracking remains independent. Movement stays horizontal; gravity, stairs and slope following are outside this movement mode.

Projects can extend the framework with left-stick turning, controller models, grabbing, teleportation and networking.

## Floor tracking

Stage is preferred, with LocalFloor as fallback. Initialization waits for valid tracking. If neither floor origin is available, the plugin reports an error, pauses joystick locomotion and retries.

Calibrate the real floor and play area in the headset system. PicoBaseStart defines the virtual floor; the XR runtime supplies physical floor tracking. Initial alignment matches headset XY and horizontal facing to the start while preserving measured eye height. Recenter or reconnect aligns against the current virtual location and facing.

## Project integration

The main configuration is **UE 5.4.4 + PICOXR 3.2.3 + Enhanced Input**, with Android ARM64 for native PICO applications. Projects configure Android SDK, signing, rendering and the XR runtime; the plugin handles player control, tracking and input.

Desktop mode supports Windows editor Play and Windows applications. VR mode is activated in a valid XR session. The right-stick mappings target PICOXR 3.2.3. Other SDKs or OpenXR streaming require their own input integration.

Source plugins require a C++ build environment and should be rebuilt when changing engine versions.

Blueprint subclasses can extend the Pawn, PlayerController and GameMode. Add PicoBaseFramework to your module dependencies when using these classes from C++. Controller components provide tracked transforms; controller meshes and interaction behavior can be added in a Pawn Blueprint.
