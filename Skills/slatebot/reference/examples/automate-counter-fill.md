# Automate: Fill counter to target value

## Goal
Click the Increment button repeatedly until the counter display reaches
a target value (default: 10).  If the counter exceeds the target, click
Decrement to adjust.  Confirm the final value matches the target.

## Prerequisites
- SlateBot instance: `CounterApp`
- Launch command: `WidgetMarkup.Show /WidgetMarkup/Samples/Counter`
- RemoteControl: `http://127.0.0.1:30010`
- Companion scripts: `Scripts/counter_fill.py`

## Widget map (discovered at exploration time)

| Widget path pattern | UMG class | Role | Click target |
|---------------------|-----------|------|--------------|
| `...CounterDisplay` | `TextBlock` | Shows current count as text | ❌ (read only) |
| `...IncrementButton` | `Button` | Increases count by 1 | ✅ |
| `...DecrementButton` | `Button` | Decreases count by 1 | ✅ |
| `...ResetButton` | `Button` | Resets count to 0 | ✅ |

> Re‑discover the transient prefix via `GetSlateBotInstances()` each run.
> Button paths are direct — no `WidgetTree_0` indirection needed for
> standard UMG `Button` widgets.
>
> Delegate: `FOnButtonClickedEvent` with `bHasBindings: true` on all
> three buttons (verified via `GetWidgetTreeDiff` first call).

## Application behavior model (discovered during exploration)

### State representation
- **Counter value:** read `CounterDisplay.Text`.  Valid values are
  `"0"` through `"20"` (inclusive).  The display updates immediately
  after each button click.
- **Button availability:** when count is at `0`, `DecrementButton`
  has `bIsEnabled: false`.  When at `20`, `IncrementButton` has
  `bIsEnabled: false`.  Read `bIsEnabled` before clicking to avoid
  sending clicks to disabled buttons.

### Interaction rules
- `IncrementButton` click → count +1 (unless at limit).
- `DecrementButton` click → count −1 (unless at limit).
- `ResetButton` click → count = 0 immediately.
- All clicks take effect within one HTTP round‑trip (no animation delay).

### Terminal conditions
- **Success:** `CounterDisplay.Text` equals the target value.
- **Recovery:** if buttons are mis‑clicked, use `ResetButton` to
  restart from 0.

### Discovered parameters
- Count range: 0–20 (inferred from observing `bIsEnabled` toggling).
- Initial value: 0 (verified by reading `CounterDisplay.Text` after
  `ResetButton` click).

## Automation procedure

### Step 1 — Launch & confirm ready
- Execute `WidgetMarkup.Show /WidgetMarkup/Samples/Counter` via
  `ExecuteConsoleCommand`.
- Poll `GetSlateBotInstances()` until `CounterApp` appears.
- Click `ResetButton` to ensure a known starting state.
- Read `CounterDisplay.Text`; expect `"0"`.

### Step 2 — Run companion script
- Execute `python Scripts/counter_fill.py --target 10`.
- The script reads `CounterDisplay.Text`, compares to target,
  clicks `IncrementButton` or `DecrementButton` as needed,
  and loops until the target is reached or a timeout expires.
- Script handles disabled‑button detection by reading `bIsEnabled`
  before each click.

### Step 3 — Verify outcome
- Read `CounterDisplay.Text` via `/remote/object/property`.
- Assert it equals `"10"`.  If not, report the actual value.

## Edge cases & recovery
- **Buttons disabled at limits:** script reads `bIsEnabled` before
  each click.  If the target button is disabled, the script adjusts
  strategy (e.g. decrement first, then increment).
- **Counter value not changing after click:** apply stuck detection
  (hash `CounterDisplay.Text` each round; abort after 3 unchanged rounds).
- **App not responding:** if `GetSlateBotInstances()` returns empty
  after launch, wait 2 s and retry up to 3 times.

## Success criteria
- `CounterDisplay.Text == "10"` after script completion.
- All clicks returned `bSuccess: True`.
- No stuck‑detection abort triggered.
