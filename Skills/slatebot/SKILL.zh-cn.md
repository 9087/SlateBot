# SlateBot 拟人化 Agent 指南

> 本质：像**一个正常人**拿到一个应用/游戏那样去**观察、理解、决定、行动、看反馈、调整**。
> 唯一的区别：你通过 RemoteControl 能**看到界面数据（控件树）**、能**模拟输入**——这是你的
> "透视 + 双手"。本指南描述的是"人怎么用工具/玩游戏"，再把这份能力接上去。

---

## 核心 · 人的行为循环

一个"人"拿到一个界面，无非是反复：

```
看(观察界面) → 理解(这是什么/什么状态/能做什么)
→ 决定(下一步) → 行动(点击/输入/拖拽)
→ 看反馈(发生了什么/对不对) → 适应(继续 or 调整) → 直到目标达成
```

每一步都**基于你看到的东西**，边做边汇报，不要憋到最后。

---

## 你的"超人感知 + 动手能力"（RemoteControl）

**除了像人一样看屏幕，你还能：**

- **看"内部状态"**：`GetWidgetTreeDiff` 拿到 `Text`/`BrushColor`/`Visibility`/`bIsEnabled`——
  相当于把 UI 的"底层数据"摊在你面前。
- **动手**：`SendClick`（左/右键+修饰键）、`SendKey`/`SendText`、`Focus`、`SendDrag`（异步）、
  `IsMouseInputPending`（防并发）。

**但要小心两个反直觉点（决定你的"人性"成色）：**

1. **别用"透视"假装是"人"**。像人一样测试的价值在于**外部观察**。你因为能看到内部数据，
   很容易偷偷依赖"人看不见的细节"——那就不是"像人"，而是"作弊"。
   → 优先用**截图/可见状态**来判断；控件树用来**交叉验证、拿精确值**，别当主判断。
2. **输入可能"假成功"**。模拟输入偶尔返回成功但**没送达**（窗口失焦、连续合成输入后失焦）。
   → **任何行动后必须看反馈，确认真的变了**，否则不算成功。

---

## 怎么"像人一样判断"（启发式）

1. **先建立语境**：先问"这是哪类东西"（游戏？表单？工具？），用常识/领域知识理解
   "这堆数字/颜色/文字意味着什么"，**再靠观察去确认**——不是跳过观察。
2. **读状态**：优先读**文本**；颜色要**先标定**（在已知状态实测阈值）再用，别猜。
3. **先确认目标还可操作（关键）**：点/执行**之前**，先判断目标是否仍处于"未完成/可用"状态
   （如格子未揭开、按钮 `bIsEnabled`、元素可见可命中）。**是才点，否则跳过**。
   - 已揭开的格子、禁用的按钮、隐藏的目标**都不用点**——此时"点了没反应"不代表 bug/没送达，
     而是**选错了目标**。判断可点性：读控件树状态 / `Visibility` / `bIsEnabled`，或看截图是否"已翻开/置灰"。
4. **试探与验证**：对"看起来可交互"且**确认可点**的东西先小成本试一下，看反应；每个动作后**回看确认**。
5. **不确定就再观察/提问**，不要硬猜；错了就回退/重来/到已知干净状态。
6. **循序渐进**：先小范围、可逆的尝试，再推进到不可逆/高风险步骤。
7. **有目标按目标推进**：始终清楚"我在达成什么、到什么算成、什么算失败"。

---

## 常见人类行为模式（照做）

| 情形 | 人会怎么做 |
|------|-----------|
| 首次接触 | 扫一眼整体、找可点的东西、试探一下 |
| 读状态 | 看数字/文字/颜色变化 |
| 执行关键步骤 | 先确认目标、再动手、完成后回看 |
| 没反应/卡住 | 再看一眼、换个方式、退一步、重新评估 |
| 出错 | 回滚/重置/回到上一个稳定状态 |
| 快成了 | 确认剩余步骤、收尾、核对最终结果 |

---

## 原语集（Skill 可引用的小工具）

`read_ui()`（截图+控件树归一）、`click(widget)`、`verify(after_action)`、
`calibrate_thresholds()`、`is_stuck()`、`wait_until()`。

---

## 快速上手 & 工具参考

### 前置：先确认能连
- 目标 App 在 UE 编辑器里运行，RemoteControl Web Server 在 `http://127.0.0.1:30010` 监听。

### 最小上手流程（照这个模板套）
```
① GetSlateBotInstances()  → 发现 App 实例 + 动态前缀
② 确认就绪              → 读一个"已知稳定"的状态，等它符合预期
③ GetWidgetTreeDiff()   → 读当前界面状态（首次=全量；之后=增量）
④ 执行动作              → SendClick()/SendKey()/SendText()/...
⑤ 回读验证              → 确认动作真的生效；没变就当失败
```

### RemoteControl 调用约定
- 端点：`PUT http://127.0.0.1:30010/remote/object/call`
- 载荷：`{"objectPath":"/Script/SlateBot.Default__SlateBotFunctionLibrary","functionName":"<fn>","parameters":{...}}`
- 返回值在 `ReturnValue` 里；执行类函数返回 `FSlateBotOperationResult`（`bSuccess`/`ErrorCode`/`ErrorMessage`）。

### 关键函数参考
| 函数 | 用途 | 关键参数 |
|------|------|---------|
| `GetSlateBotInstances` | 发现运行中的 SlateBot 实例 | 无 |
| `GetWidgetTreeDiff` | 读控件树状态（首次全量，之后增量 diff） | `InstanceName` |
| `ResetWidgetTreeCache` | 重置某实例 diff 缓存（下次=全量） | `InstanceName` |
| `WaitForWidgetTreeDiff` | 阻塞等待状态变化（用于"等反馈"） | `InstanceName`,`TimeoutMs` |
| `SendClick` | 模拟点击 | `Widget`、`Options` |
| `SendKey` / `SendText` | 键盘 / 文本输入（发到当前焦点） | `Key` / `Text` |
| `Focus` | 聚焦某控件（配合 SendText） | `Widget` |
| `SendDrag` | 拖拽（**异步**：返回=已调度，非已完成） | `Widget`、`Options` |
| `CaptureSlateBotScreenshot` | 截图存 PNG（供视觉模型看） | `InstanceName`、`OutputPath` |
| `IsMouseInputPending` | 是否有鼠标输入进行中 | 无 |

### 对象路径 & 动态前缀
- 用 **UMG 对象路径**引用控件，如 `/Engine/Transient.World_3:App_C_0.WidgetTree_0.SomeWidget`。
- 前缀（到 `.WidgetTree_0` 为止）是**瞬时的**，每次重开会变 → 用 `GetSlateBotInstances()` 动态取。
- 复合控件：`<外罩>.WidgetTree_0.<子控件>`；基础控件（`Button`/`TextBlock`/`Border`）直接可用。

### 最小可跑例子（Python，任意 HTTP 客户端同理）
```python
import requests
BASE = "http://127.0.0.1:30010/remote/object/call"
FN   = "/Script/SlateBot.Default__SlateBotFunctionLibrary"
def rpc(fn, **p):
    r = requests.put(BASE, json={"objectPath": FN, "functionName": fn, "parameters": p}, timeout=15)
    return r.json().get("ReturnValue")

insts = rpc("GetSlateBotInstances")                     # ① 发现实例
inst  = insts[0]["InstanceName"]
prefix = str(insts[0]["SlateBot"]).rsplit(".SlateBot_", 1)[0]
tree = rpc("GetWidgetTreeDiff", InstanceName=inst)      # ② 读状态
# ③ 点一个控件
rpc("SendClick", Widget=f"{prefix}.Cell_1_1", Options={
    "Button": {"KeyName": "LeftMouseButton"}, "ClickType": "Single",
    "ModifierKeys": {"bControl": False}, "RelativePosition": {"X": 0.5, "Y": 0.5}})
# ④ 回读验证
tree2 = rpc("GetWidgetTreeDiff", InstanceName=inst)
```

> 提示：**不要用 `/remote/batch`**（会导致 UE 编辑器崩溃，已知 bug）。并发鼠标输入记得查
> `IsMouseInputPending`。

---

## 法则（最底层，任何时候不破）

- **纯黑盒**：不读目标应用源码；用 RemoteControl + 截图观察。
- **行动必验证**：动作没在界面上被确认生效，就不算做过。
- **像人，不读心**：判断以"人看得到"的外部状态为准；内部数据只做补充/交叉验证。
