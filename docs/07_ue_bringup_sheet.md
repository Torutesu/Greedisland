# UE5.8 Bring-up Sheet

最終更新: 2026-08-12

## 目的

UE5.8 側で `UnrealProject/Greeisland.uproject` を開いたあと、最短で 1 ゾーン MVP を確認するための実作業シート。

このシートは [UE5.8 デバッグHUD接続手順](/Users/torutano/Documents/Greeisland/docs/06_ue_debug_hud_setup.md) の要点だけを、実施順で並べ直したもの。レベルにBootstrapActor、EventActor、Widget Blueprintを手置きしていない場合でも、C++のDebugGameModeが1ゾーンとNative HUDを補完する。

## 0. 事前条件

- `UnrealProject/Greeisland.uproject` が開ける
- `Project Settings > Game > Greeisland` の `CardJsonPath` / `EventJsonPath` が既定値のままか、正しい場所を指している
- `DefaultGame.ini` の既定値では `ProjectDir` 基準で以下を読む
  - `../data/cards/cards.mvp.json`
  - `../data/events/events.mvp.json`
- `DefaultEngine.ini` の既定マップは `/Engine/Maps/Entry`。新規レベルを作らなくても、C++ GameModeがPlay開始時にMVPゾーンを生成する。
- `PlayerStart` が無い空レベルでも、C++ GameModeがMVP用の開始地点を補完する。

エディタ起動前に、ターミナル側ではこれを 1 回通しておくと速い:

```bash
python3 scripts/validate_events/ue_editor_preflight_smoke_test.py
```

UEに入る前の宣言形チェックも実行する:

```bash
python3 scripts/validate_events/ue_uht_surface_smoke_test.py
```

UE が入っているマシンでは、次のコマンドで `projectfiles / build-editor / build-game / open-editor` の実行コマンドを確認できる:

```bash
python3 scripts/ue_build_helper.py --action print-plan
```

不足ツールも含めて一気に見たいときは:

```bash
python3 scripts/ue_build_helper.py --action doctor
```

UE が見つかっていて、そのまま projectfiles -> build-editor -> open-editor を流したいときは:

```bash
python3 scripts/ue_build_helper.py --action bootstrap-editor
```

Editorを介したStandalone相当の起動だけを行う場合は:

```bash
python3 scripts/ue_build_helper.py --action run-game
```

これは `UnrealEditor <uproject> -game -log` を実行し、既定のEntryマップとC++ GameModeをそのまま起動する。

実機確認のチェック順までまとめて見たいときは:

```bash
python3 scripts/ue_build_helper.py --action checklist
```

MVP完了条件ごとの現状棚卸しを先に見たいときは:

```bash
python3 scripts/mvp_audit.py --format markdown
```

確認ログの雛形をその場で出したいときは:

```bash
python3 scripts/ue_build_helper.py --action report-template
```

この雛形には `MVP Audit Snapshot` も入るので、ロジック証跡と実機未確認項目を同じレポートで追える。

ファイルとして残すなら:

```bash
python3 scripts/ue_build_helper.py --action report-template --output artifacts/ue-runtime-checks/first-pass.md
```

監査、チェック順、doctor、レポートをまとめて吐きたいときは:

```bash
python3 scripts/ue_build_helper.py --action write-verification-pack
```

生成された pack の `README.md` から見始めると、読む順番と実行順がそのまま分かる。
`ue-editor-preflight-output.txt` と `mvp-audit-output.md` には、その場で採れた事前証跡も同梱される。

## 1. 作る Blueprint アセット

これは見た目を作り込む場合の任意手順。最短のMVP確認では、C++のDebugGameModeがBootstrapActor、5地点、床、Native HUDを自動生成するため、Blueprintアセットを作らずに進められる。

`UGreeislandDebugHudWidget.GetRecommendedBlueprintAssets()` の返り値と同じ内容。

1. `BP_GreeislandDebugHudWidget`
   - Parent: `UGreeislandDebugHudWidget`
   - 最低限、診断、イベント一覧、手札、所持カード、ログを表示する

2. `BP_GreeislandDebugHud`
   - Parent: `AGreeislandDebugHud`
   - `DebugHudWidgetClass` に `BP_GreeislandDebugHudWidget` を設定する

3. `BP_GreeislandDebugGameMode`
   - Parent: `AGreeislandDebugGameMode`
   - 必要なら `HUD Class` を `BP_GreeislandDebugHud` に差し替える

4. `BP_GreeislandBootstrapActor`
   - Parent: `AGreeislandBootstrapActor`
   - レベルに 1 個だけ置く
   - `bUseProjectSettingsDefaults` を有効にする

5. `BP_GreeislandEventActor`
   - Parent: `AGreeislandEventActor`
   - イベント地点ごとに複製し、`EventId` を設定する

6. `BP_GreeislandDebugCharacter`
   - Parent: `AGreeislandDebugCharacter`
   - 必要なら見た目メッシュを設定する

## 2. レベルに置くもの

Native fallbackを使う場合は、このセクションの手置きも不要。手作業で配置した場合は、C++の自動生成は不足分だけを補完する。

1. `BP_GreeislandBootstrapActor`
   - 1 個だけ

2. `BP_GreeislandEventActor`
   - 以下の `EventId` を最低 1 個ずつ配置する
   - `event_wake_cache_001`
   - `event_contract_broker_001`
   - `event_silent_shrine_001`
   - `event_ridge_scout_001`
   - `event_proxy_gate_001`

3. `BP_GreeislandDebugCharacter`
   - Play 開始地点に出せるようにする

## 3. World Settings

最低限これだけ確認する。

- `GameMode Override = BP_GreeislandDebugGameMode`
- `BP_GreeislandDebugHud` が `HUDClass` に入っている

## 4. Widget に置く最小セクション

`UGreeislandDebugHudWidget.GetRecommendedHudChecklist()` の返り値と同じ内容。

`UGreeislandDebugHudWidget.GetRecommendedHudPanels()` を見ると、下のセクションをどのパネル単位で分けるかも分かる。

### Status

- `SessionStatusRows`
- `CurrentObjectiveAction`
- `CurrentSnapshot.ActiveEventDisplayName`
- `LastBootstrapDiagnostics.Issues`
- `FocusedEventDisplayName`

### Lists

- `EventViewData`
- `HandCardViewData`
- `OwnedCardViewData`
- `CurrentSnapshot.RecentLogLines`
- `LastBootstrapDiagnostics.ExpectedEventPlacements`
- `LastBootstrapDiagnostics.ExpectedEventRoute`
- `EventActorStatusViewData`
- `WalkthroughProgress`
- `RecommendedHudActions`
- `HudActionStates`
- `HudActionButtons`
- `VerificationChecks`

行表示の近道:

- `HandCardViewData`
  - タイトル: `DisplayName`
  - 補足1: `StateSummary`
  - 補足2: `DetailSummary`
  - 行ボタン: `PrimaryActionLabel`
- `OwnedCardViewData`
  - タイトル: `DisplayName`
  - 補足1: `StateSummary`
  - 補足2: `DetailSummary`
  - 行ボタン: `PrimaryActionLabel` があるときだけ表示
- `EventViewData`
  - タイトル: `DisplayName`
  - 補足1: `StatusSummary`
  - 補足2: `DetailSummary`
  - 行ボタン: `PrimaryActionLabel` があるときだけ表示

起動直後は `WalkthroughProgress` と `VerificationChecks` を先頭に置くと、どこで止まっているか見分けやすい。

`SessionStatusRows` を最上段の repeater/list にそのまま流すと、Blueprint 側で文言を組み立てずに済む。

- `Label`
  - 行見出し
- `Value`
  - 現在値
- `StatusLabel`
  - `Ready` / `Clean` / `Live` / `In Range` などの短い状態表示
- `Detail`
  - 補足説明
- `bHealthy`
  - 色替えやアイコン切り替え用

`CurrentObjectiveAction` をその横または直下に置くと、「次に押すべき 1 ボタン」を出しやすい。

- `StepLabel`
  - いまの walkthrough ステップ名
- `ActionLabel`
  - いま押すべきボタン名
- `AvailabilityLabel`
  - 押せるかどうか
- `DefaultNameArgument`
  - `ExecuteHudActionById` に渡す既定 `Name`

`UGreeislandDebugHudWidget` は既定で `0.2` 秒ごとに自動更新されるので、探索中に `Focused Event` や `Interact Focused` の有効状態が追従する前提でよい。

推奨パネル順:

1. `session_status`
2. `event_reconciliation`
3. `exploration_state`
4. `combat_hand`
5. `collection`
6. `actions`
7. `ai_debug` (任意)

推奨アクションの置き方:

- `actions` パネル
  - `Bootstrap`
  - `New Session`
  - `Restore`
  - `Save`
  - `Interact Focused`
  - `Resolve Active`
  - `Start Combat`
  - `Enemy Turn`
  - `Grant Card`
- `ai_debug` パネル
  - `Build AI Request`
  - `Fallback AI`
  - `Apply AI`

`HudActionStates` を見て、各ボタンの enabled/disabled を切り替える。

`HudActionButtons` を使うと、actions パネルはこの配列 1 本で描ける。

- `Label`
  - ボタン表示名
- `bEnabled`
  - enabled/disabled 切り替え
- `AvailabilityLabel`
  - 状態表示
- `Detail`
  - 補足表示
- `DefaultNameArgument` / `DefaultStringArgument` / `bDefaultFlag`
  - `ExecuteHudActionById` にそのまま渡す既定引数

ボタン実行の近道:

- `RecommendedHudActions[i].ActionId`
  - ボタンの識別子
- `ExecuteHudActionById(ActionId, OptionalNameArgument, OptionalStringArgument, bOptionalFlag)`
  - Blueprint 側で action ごとの分岐を書かずに押下処理を流せる
- `HandCardViewData[i].PrimaryActionId` / `EventViewData[i].PrimaryActionId`
  - 行ごとのボタン押下をそのまま流せる
- `grant_developer_card`
  - `OptionalNameArgument` を空にすると `DefaultDeveloperGrantCardId` を使う

`VerificationChecks` は、HUD 上の最終確認欄として使う。

- `StatusLabel`
  - `Pass` / `Needs Work`
- `Detail`
  - 何が足りないかの短い説明
- `bRequiredForMinimalLoop`
  - MVP の必須項目だけを先に見たいときのフィルタ用

### Actions

- `BootstrapSessionFromActor`
- `InitializeNewSession`
- `RestoreSession`
- `SaveSession`
- `ResolveActiveEvent`
- `StartCombatForActiveEvent`
- `RunEnemyTurn`
- `GrantDeveloperCard`

任意:

- `BuildAiRequestForActiveEvent`
- `BuildFallbackAiResponseForActiveEvent`

## 5. 起動直後の診断

Play 後、まず以下だけ確認する。

1. `LastBootstrapDiagnostics.Issues` が空
2. `LastBootstrapDiagnostics.MissingEventActorIds` が空
3. `LastBootstrapDiagnostics.DuplicateEventActorIds` が空
4. `LastBootstrapDiagnostics.ExpectedEventPlacements` の `PlacementCount` が全イベントで 1
5. `LastBootstrapDiagnostics.ExpectedEventRoute` の `RouteDepth` が
   - `event_wake_cache_001 = 0`
   - `event_contract_broker_001 = 1`
   - `event_silent_shrine_001 = 2`
   - `event_ridge_scout_001 = 2`
   - `event_proxy_gate_001 = 3`

ここで崩れていたら、先にレベル配置か JSON パスを直す。

## 5.5. 配置するときの並べ方

`ExpectedEventRoute` は、レベル上のざっくりした導線メモとして使う。

- `RouteDepth`
  - 0 から右方向または奥方向へ 1 列ずつ進める
- `SuggestedLane`
  - 同じ `RouteDepth` の中で上下にずらす順番の目安
- `IncomingEventIds` / `NextEventIds`
  - 分岐と合流の確認用

MVP の最小配置イメージ:

- Depth 0: `event_wake_cache_001`
- Depth 1: `event_contract_broker_001`
- Depth 2: `event_silent_shrine_001`, `event_ridge_scout_001`
- Depth 3: `event_proxy_gate_001`

`WalkthroughProgress` は、HUD 上の作業順ガイドとして使う。

- `bCurrentFocus == true`
  - いま次に触るべき手順
- `StatusLabel`
  - `Completed` / `Next Up` / `Available` / `Locked`
- `Detail`
  - なにを確認すれば前に進むかの短いメモ

`EventActorStatusViewData` は、レベル配置とセッション進行の照合に使う。

- `bHasActorPlacement == false`
  - JSON 上は存在するがレベルに置けていない
- `bAvailable == true` かつ `bIsFocused == false`
  - 進行上は触れるはずだが、いま近くにいない
- `bIsInteractableNow == true`
  - そのまま `E` で進められる候補
- `StatusSummary`
  - 一覧でざっと見る用の短い状態文字列

## 6. MVP 一周の確認順

`UGreeislandDebugHudWidget.GetRecommendedWalkthrough()` の返り値と同じ内容。

1. Bootstrap Session
   - `BootstrapSessionFromActor` または `InitializeNewSession`
   - 成功条件: `CurrentSnapshot.bHasInitializedSession == true`

2. Wake Cache
   - `event_wake_cache_001`
   - 成功条件: starter cards が `OwnedCardViewData` に出る

3. Contract Broker
   - `event_contract_broker_001`
   - 成功条件: `item_contract_001` と `rule_party_proxy_001` が揃う

4. Silent Shrine
   - `event_silent_shrine_001`
   - 成功条件: quest completion が反映される

5. Ridge Scout Battle
   - `event_ridge_scout_001`
   - 成功条件: `con_four_party_001` が獲得される

6. Proxy Gate
   - `event_proxy_gate_001`
   - 成功条件: `key_zone_core_001` を獲得し、`bZoneCleared == true`

7. Save And Restore
   - `SaveSession` のあと `RestoreSession`
   - 成功条件: 所持カードとクリア状態が維持される

## 7. つまずきやすい点

- `Issues` に JSON path エラー:
  - `Project Settings > Game > Greeisland` のパスを見直す

- `MissingEventActorIds` が残る:
  - `BP_GreeislandEventActor` を足して `EventId` を合わせる

- `DuplicateEventActorIds` が出る:
  - 同じ `EventId` を持つ Actor を整理する

- HUD が出ない:
  - `GameMode Override`
  - `HUDClass`
  - `DebugHudWidgetClass`
  の 3 点を順に確認する

- Focused Event やボタン状態が歩いても変わらない:
  - `BP_GreeislandDebugHudWidget` 側で `bAutoRefreshPresentation` を無効にしていないか確認する
  - `AutoRefreshIntervalSeconds` を極端に大きくしていないか確認する

- プレイヤーが動かない:
  - `DefaultInput.ini`
  - `DefaultPawnClass`
  - `BP_GreeislandDebugCharacter`
  を確認する
