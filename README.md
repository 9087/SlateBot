# SlateBot

Automation plugin for UE5 UMG / Slate UIs.  Wraps widget trees inside named
**SlateBot instances** and exposes input‑simulation primitives through a
`UBlueprintFunctionLibrary`, callable via **Web Remote Control** (port 30010).

## Features

- **Instance registry** — locate any SlateBot by its assigned `InstanceName`.
- **Input primitives** — `SendClick` (single/double, any button, modifier keys, position inside
  the widget), `SendMouseMove` (hover / tooltip), `SendMouseWheel`, `SendKey` / `SendText`
  (with `Focus`), and `SendDrag` (scheduled frame by frame, never blocks the game thread).
- **Observation** — `GetWidgetTreeDiff` (full tree first, then `Add`/`Change`/`Delete` deltas),
  `GetWidgetGeometry` (absolute and local size, DPI scale), and `CaptureSlateBotScreenshot`
  (renders the instance off-screen to PNG, so an occluded or minimised window still works).
- **ListView introspection** — `GetListViewInfo`, `GetListEntryInfo` (read-only row lookup),
  `ScrollToListEntry`, `ScrollToListEntryAndSendClick`.
- **White‑box scripting** — combine RemoteControl property reads with
  the input primitives to build end‑to‑end automation scripts in Python (or any HTTP
  client language).

## Quick start

1. Place a **SlateBot** widget as the root of your UI in the UMG designer,
   then set its `InstanceName` property (e.g. `"MyApp"`).
2. Build and open the editor.  Make sure **Remote Control Web Server** is
   enabled (default port 30010).
3. Call `GetSlateBotInstances` on `/Script/SlateBot.Default__SlateBotFunctionLibrary`
   to discover running instances.
4. Use `SendClick` with the target widget's object path to simulate input.
5. Read state with `GetWidgetTreeDiff`, and `CaptureSlateBotScreenshot` when a picture is easier
   to judge than a property value.

## Scripting

See [`Skills/slatebot/SKILL.md`](Skills/slatebot/SKILL.md)
for the full two‑phase workflow (exploration → automation) with code
templates and reference tables.
