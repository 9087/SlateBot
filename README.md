# SlateBot

Automation plugin for UE5 UMG / Slate UIs.  Wraps widget trees inside named
**SlateBot instances** and exposes input‑simulation primitives through a
`UBlueprintFunctionLibrary`, callable via **Web Remote Control** (port 30010).

## Features

- **Instance registry** — locate any SlateBot by its assigned `InstanceName`.
- **SendClick** — simulate left/right mouse clicks on any `UWidget` by object
  path, with configurable position, modifier keys, and single/double click.
- **White‑box scripting** — combine RemoteControl property reads with
  `SendClick` to build end‑to‑end automation scripts in Python (or any HTTP
  client language).

## Quick start

1. Place a **SlateBot** widget as the root of your UI in the UMG designer,
   then set its `InstanceName` property (e.g. `"MyApp"`).
2. Build and open the editor.  Make sure **Remote Control Web Server** is
   enabled (default port 30010).
3. Call `GetSlateBotInstances` on `/Script/SlateBot.Default__SlateBotFunctionLibrary`
   to discover running instances.
4. Use `SendClick` with the target widget's object path to simulate input.

## Scripting

See [`Skills/slatebot-scripting/SKILL.md`](Skills/slatebot-scripting/SKILL.md)
for the full two‑phase workflow (exploration → automation) with code
templates and reference tables.
