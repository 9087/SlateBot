# SlateBot — Anthropomorphic Agent Guide

> Act like a **human** using/playing any UE5 UMG/Slate UI: observe, understand, decide,
> act, watch for feedback, adapt. The only difference: via RemoteControl you can also
> **read the widget tree** (an "X-ray" into UI data) and **simulate input** ("hands").
> App-agnostic — specific apps appear only as illustrations.
>
> 中文版见 `SKILL.zh-cn.md`。

---

## The human loop

```
look(see the UI) → understand(what/state/what can I do)
→ decide(next step) → act(click/type/drag)
→ see feedback(did it change? right?) → adapt(continue or adjust) → until goal
```

Report as you go; don't wait until the end.

---

## Your extra powers (RemoteControl)

- **Read UI state**: `GetWidgetTreeDiff` → `Text`/`BrushColor`/`Visibility`/`bIsEnabled`.
- **Act**: `SendClick` (left/right + modifiers), `SendKey`/`SendText`, `Focus`, `SendDrag`
  (**async**: returns = scheduled, not done), `IsMouseInputPending` (avoid concurrent mouse).

**Two counter-intuitive traps (this is what keeps you "human"):**

1. **Don't fake being human with the "X-ray".** The value of human-like testing is *external
   observation*. If you rely on details a real user can't see, you're cheating.
   → Judge from **visible / screenshot** state; use the widget tree only to **cross-check / get exact values**.
2. **Input can "false-succeed".** A simulated input may report success but not land
   (window lost focus, after many synthetic inputs).
   → **Always re-read after an action; if nothing changed, it did NOT succeed.**

---

## How to judge like a human (heuristics)

1. **Build context first**: ask "what kind of thing is this" (game? form? tool?), use common sense
   to interpret numbers/colors/text, **then confirm via observation** — don't skip observing.
2. **Read state**: prefer **text**; for colors, **calibrate** (measure on a known state) before use.
3. **Check the target is still actionable (key)**: BEFORE clicking/acting, confirm the target is
   still in an "unfinished / enabled / visible" state (e.g. cell not yet revealed, button `bIsEnabled`,
   element visible & hit-testable). **Only act if actionable; otherwise skip it.** Clicking an
   already-revealed cell, a disabled button, or a hidden element does nothing — that is NOT a
   "false success / not delivered" bug; it's picking the wrong target. Judge via widget-tree state,
   `Visibility`/`bIsEnabled`, or the screenshot (revealed/greyed-looking).
4. **Probe & verify**: on a target you've confirmed is actionable, do a cheap, reversible interaction; re-read to confirm.
5. **If unsure, observe/ask again** — don't guess hard; on error, roll back / reset / return to a known state.
6. **Go stepwise**: small, reversible steps first, then irreversible/high-risk ones.
7. **Know your goal**: what counts as done / failed; drive toward it.

---

## Quick start (template)

```
① GetSlateBotInstances()  → find app instance + dynamic prefix
② Confirm ready           → read a "known stable" state; wait until it matches
③ GetWidgetTreeDiff()     → read current UI state (first = full; then incremental)
④ Act                     → SendClick()/SendKey()/SendText()/...
⑤ Re-read to verify       → confirm it changed; if not, treat as failure
```

**RemoteControl call**: `PUT http://127.0.0.1:30010/remote/object/call`
Body: `{"objectPath":"/Script/SlateBot.Default__SlateBotFunctionLibrary","functionName":"<fn>","parameters":{...}}`
Result in `ReturnValue` (`FSlateBotOperationResult`: `bSuccess`/`ErrorCode`/`ErrorMessage`).

**Key functions**: `GetSlateBotInstances`, `GetWidgetTreeDiff`, `ResetWidgetTreeCache`,
`SendClick`, `SendMouseMove`, `SendMouseWheel`, `SendKey`, `SendText`, `Focus`,
`SendDrag` (async), `CaptureSlateBotScreenshot`, `CloseSlateBotWindow`, `IsMouseInputPending`.

**Widget paths**: use UMG object path; the prefix up to `.WidgetTree_0` is **transient** —
get it dynamically. Composite widgets: `<outer>.WidgetTree_0.<child>`; base widgets direct.

**Minimal example (Python / any HTTP client)**:
```python
import requests
BASE="http://127.0.0.1:30010/remote/object/call"; FN="/Script/SlateBot.Default__SlateBotFunctionLibrary"
def rpc(fn, **p): return requests.put(BASE, json={"objectPath":FN,"functionName":fn,"parameters":p}, timeout=15).json().get("ReturnValue")
insts=rpc("GetSlateBotInstances")                     # ① find app + prefix
inst=insts[0]["InstanceName"]; prefix=str(insts[0]["SlateBot"]).rsplit(".SlateBot_",1)[0]
tree=rpc("GetWidgetTreeDiff", InstanceName=inst)      # ② read state
rpc("SendClick", Widget=f"{prefix}.Cell_1_1", Options={"Button":{"KeyName":"LeftMouseButton"},"ClickType":"Single","ModifierKeys":{"bControl":False},"RelativePosition":{"X":0.5,"Y":0.5}})  # ③ act
tree2=rpc("GetWidgetTreeDiff", InstanceName=inst)     # ④ verify
```
> Don't use `/remote/batch` (crashes the editor). Check `IsMouseInputPending` before concurrent input.

---

## Waiting and polling (there is no blocking wait)

- RemoteControl calls run **on the game thread**: waiting inside the engine means nobody ticks, so
  the UI can never change (an old `WaitForWidgetTreeDiff` died exactly that way and is gone).
- Waiting on the **caller** side (your script / HTTP client) is safe — so "sleep, then re-read"
  belongs in the script, not in a new engine-side wait function.
- To "wait for feedback": keep a local model of the tree and fold in `GetWidgetTreeDiff`'s
  `Add`/`Change`/`Delete` until two consecutive reads report no changes (settled).
- Async actions are separate: `SendDrag` returning means **scheduled**; poll `IsMouseInputPending`
  until it is false, then read the result.

---

## Troubleshooting (when a call reports success but the UI didn't change)

| # | Check | How |
|---|-------|-----|
| 1 | Is the target ready? | Read a known-stable state and wait until it matches |
| 2 | **Is the target still actionable?** | Read `Visibility`/`bIsEnabled`, or check screenshot (already revealed / greyed). Skip finished/disabled/hidden targets |
| 3 | Does the target have a handler? | Check `GetWidgetTreeDiff` node `Delegates` (`bHasBindings: true`) |
| 4 | Deferred update? | Wait 1–2 s and re-read; some apps update on tick |
| 5 | Reading the right property? | Prefer `Text`; for colors, calibrate then range-match (below) |
| 6 | Is the target window active? | Minimized/behind window won't route mouse input; `BringToFront` may not suffice — reload the window |

> If still nothing after all checks, report target-path, before/after values, and delegate/bind state.

## Visual (color) state reading

- Actual RGB can be shifted by Slate visual multipliers (pressed/hovered, often ×1.05–1.20).
- Calibrate a baseline on a **known state**, then classify with a **per-channel ±0.10 range** rather than exact values.

## Batch-observation performance

- Prefer `GetWidgetTreeDiff` (1 call = full tree, then incremental diffs) over many per-widget/per-property calls.

---

## Common human behaviors (do the same)

| Situation | What a person does |
|-----------|--------------------|
| First contact | Skim overall, find clickable things, probe |
| Read state | Watch numbers/text/color changes |
| Critical step | Confirm target, act, then re-check |
| Nothing / stuck | Look again, try differently, step back, re-evaluate |
| Error | Roll back / reset / return to a stable state |
| Almost done | Confirm remaining steps, finish, verify final result |

---

## Primitives

`read_ui()` (screenshot+tree → normalized state), `click(widget)`, `verify(after_action)`,
`calibrate_thresholds()`, `is_stuck()`, `wait_until()`.

---

## Laws (never break)

- **Pure black box**: don't read target-app source; observe via RemoteControl + screenshots.
- **Action ⇒ verify**: an action isn't done until the UI confirms it took effect.
- **Be human, don't read minds**: judge from what a human can *see*; UI internals are only
  a cross-check / exact-value helper.

---

> Older scripting-focused draft preserved under `reference/`.
