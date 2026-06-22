# UE5.8 デバッグHUD接続手順

最終更新: 2026-06-21

## 目的

このリポジトリのC++ロジックを、UE5.8上で最短で触れるようにする。

最短で現物確認に入るときは、先に [UE5.8 Bring-up Sheet](/Users/torutano/Documents/Greeisland/docs/07_ue_bringup_sheet.md) を見ると流れを追いやすい。

エディタを開く前に `python3 scripts/validate_events/ue_editor_preflight_smoke_test.py` を通しておくと、`uproject` / `Target.cs` / `Build.cs` / `DefaultGame.ini` / `DefaultInput.ini` / JSON 配置の前提をまとめて確認できる。

UE が入っている環境に持っていったら、`python3 scripts/ue_build_helper.py --action print-plan` で `GenerateProjectFiles.sh` / `Build.sh` / `UnrealEditor` の解決結果を先に確認できる。

`python3 scripts/ue_build_helper.py --action doctor` を使うと、見つかっていないツールと次に叩くべきコマンドをまとめて出せる。

UE が見つかっているなら、`python3 scripts/ue_build_helper.py --action bootstrap-editor` で `projectfiles -> build-editor -> open-editor` をそのまま順に流せる。

`python3 scripts/ue_build_helper.py --action checklist` で、エディタ起動後に何をどの順で確認するかの実機チェック順も出せる。

`python3 scripts/ue_build_helper.py --action report-template` を使うと、実機確認の結果を書き残す Markdown 雛形をそのまま出せる。`--output artifacts/ue-runtime-checks/first-pass.md` のように渡せばファイル生成もできる。

## すでに用意されているC++クラス

- `UGreeislandGameSubsystem`
  - ゲーム進行、保存復元、AI反映、UI向けスナップショット
- `UGreeislandDebugHudWidget`
  - Blueprintで見た目を被せる前提の `UUserWidget`
  - 近接中の `FocusedEventId` / `FocusedEventDisplayName` と `InteractWithFocusedEvent` を取得可能
  - `BuildAiRequestForActiveEvent` で Claude API 等へ送る前段の入力を取得可能
  - `BuildFallbackAiResponseForActiveEvent` と `ValidateAiResponseForActiveEvent` で固定文面フォールバックと検証結果を取得可能
  - `GrantDeveloperCard` で任意カードを開発用に所持/デッキへ注入できる
  - `BootstrapSessionFromActor` と `LastBootstrapDiagnostics` でレベル上の BootstrapActor をHUDから直接確認できる
  - `GetRecommendedHudChecklist` で最小HUDに置くべき表示項目とボタン一覧を取得できる
  - `GetRecommendedHudPanels` で最小HUDをどのパネルに分けるか取得できる
  - `GetRecommendedHudActions` と `GetHudActionStates` でボタン定義と有効/無効状態を取得できる
  - `GetHudActionButtons` で actions パネル用の結合済みボタン配列を取得できる
  - `GetCurrentObjectiveAction` で walkthrough の現在ステップに対応する「いま押すべき一手」を取得できる
  - `ExecuteHudActionById` で `ActionId` から主要ボタン処理をそのまま実行できる
  - `GetVerificationChecks` でMVP bring-up の pass/fail 一覧を取得できる
  - `GetSessionStatusRows` で最上段パネル用の行データをそのまま取得できる
  - `GetRecommendedWalkthrough` で MVP 一周の推奨確認手順を取得できる
  - `GetWalkthroughProgress` で MVP 一周の進捗状態を取得できる
  - `GetEventActorStatusViewData` で EventActor の配置状態と進行状態の食い違いを一覧確認できる
  - `GetRecommendedBlueprintAssets` で作成すべき BP アセットと親クラス、最低設定を取得できる
  - `bAutoRefreshPresentation` が既定で有効なので、歩行中の focused event や action state も HUD 上で自動追従する
- `AGreeislandDebugHud`
  - `DebugHudWidgetClass` を生成してViewportへ追加する `AHUD`
- `AGreeislandDebugGameMode`
  - `DefaultPawnClass`、`PlayerControllerClass`、`HUDClass` をデバッグ向けに設定した `AGameModeBase`
- `AGreeislandDebugPlayerController`
  - マウス表示とクリックを有効にした `APlayerController`
- `AGreeislandDebugCharacter`
  - `WASD` 移動、マウス視点、`E` インタラクトを持つ最小プレイアブルキャラクター
- `AGreeislandBootstrapActor`
  - `bUseProjectSettingsDefaults` が有効なら `Project Settings > Game > Greeisland` の JSON / save 設定をそのまま使う
  - `GetBootstrapDiagnostics` で JSON パス解決、save slot 有無、Subsystem 利用可否を事前確認できる
  - `MissingEventActorIds` / `DuplicateEventActorIds` / `UnexpectedEventActorIds` でマップ上のイベント配置ミスを確認できる
  - `ExpectedEventPlacements` で JSON 定義順の配置チェックリストをそのまま UI に出せる
  - `ExpectedEventRoute` で分岐込みの推奨導線を UI に出せる

## UE側で作るもの

1. `BP_GreeislandDebugHudWidget`
   - Parent Class: `UGreeislandDebugHudWidget`

2. `BP_GreeislandDebugHud`
   - Parent Class: `AGreeislandDebugHud`
   - `DebugHudWidgetClass` に `BP_GreeislandDebugHudWidget` を設定

3. `BP_GreeislandDebugGameMode`
   - Parent Class: `AGreeislandDebugGameMode`
   - 必要なら `HUD Class` を `BP_GreeislandDebugHud` に差し替え

4. `BP_GreeislandBootstrapActor`
   - Parent Class: `AGreeislandBootstrapActor`
   - マップに1個置く

5. `BP_GreeislandEventActor`
   - Parent Class: `AGreeislandEventActor`
   - Eventごとに複製して `EventId` を設定

6. `BP_GreeislandDebugCharacter`
   - Parent Class: `AGreeislandDebugCharacter`
   - 必要ならメッシュやカメラ距離を調整

## World Settings / Project Settings

最短では、対象マップの `World Settings` で `GameMode Override` を `BP_GreeislandDebugGameMode` にする。

常時使いたい場合は `Project Settings > Maps & Modes` で Default GameMode に設定する。

補足:

- `UnrealProject/Config/DefaultGame.ini` では C++ の `AGreeislandDebugGameMode` が `GlobalDefaultGameMode` に設定済み
- `UnrealProject/Config/DefaultInput.ini` では `WASD`、マウス視点、`E` インタラクトが設定済み
- `CardJsonPath` / `EventJsonPath` の既定値は `ProjectDir` から見て `../data/cards/cards.mvp.json` と `../data/events/events.mvp.json`
- Blueprint版を使う場合は `Project Settings` または `World Settings` で `BP_GreeislandDebugGameMode` へ差し替える
- `Project Settings > Game > Greeisland` では `CardJsonPath`、`EventJsonPath`、`SaveSlotName`、`DefaultOpeningDrawCount` などを変更できる

## Widget Blueprint の最初の配置案

最初は以下だけでよい。

- 現在イベント名
- AvailableEvent の一覧
- HandCardViewData の一覧
- OwnedCardViewData の一覧
- RecentLogLines の一覧
- ボタン
  - `InitializeNewSession`
  - `RestoreSession`
  - `SaveSession`
  - `ResolveActiveEvent`
  - `StartCombatForActiveEvent`
  - `RunEnemyTurn`
  - `GrantDeveloperCard`

## Blueprintで使うデータ

`UGreeislandDebugHudWidget` から参照できるもの:

- `CurrentSnapshot`
- `PlayableCombatCardIds`
- `OwnedCardViewData`
- `HandCardViewData`
- `EventViewData`
- `LastActionResult`
- `FocusedEventId`
- `FocusedEventDisplayName`
- `LastBuiltAiRequest`
- `LastAiResponse`
- `LastAiValidationResult`
- `LastBootstrapDiagnostics`

`HandCardViewData` / `OwnedCardViewData` では追加で以下を読める:

- `EffectivePartySize`
- `UnplayableReasons`

`RecommendedHudChecklist` の各要素では以下を読める:

- `Group`
- `Label`
- `BindingHint`
- `bRecommendedByDefault`

`RecommendedWalkthrough` の各要素では以下を読める:

- `Order`
- `Label`
- `EventId`
- `ActionHint`
- `SuccessHint`

`RecommendedBlueprintAssets` の各要素では以下を読める:

- `AssetName`
- `ParentClassName`
- `RequiredSetup`
- `bRequiredForMinimalLoop`

`WalkthroughProgress` の各要素では以下を読める:

- `Order`
- `Label`
- `EventId`
- `bCompleted`
- `bCurrentFocus`
- `bEventAvailable`
- `StatusLabel`
- `Detail`

`RecommendedHudPanels` の各要素では以下を読める:

- `PanelId`
- `Title`
- `Purpose`
- `SuggestedWidgetType`
- `Priority`
- `bRecommendedForMinimalLoop`
- `BindingHints`

`RecommendedHudActions` の各要素では以下を読める:

- `ActionId`
- `Label`
- `MethodName`
- `PanelId`
- `Purpose`
- `EnableWhen`
- `InputHint`
- `Priority`
- `bRecommendedForMinimalLoop`

`HudActionStates` の各要素では以下を読める:

- `ActionId`
- `bEnabled`
- `AvailabilityLabel`
- `Detail`

`HudActionButtons` の各要素では以下を読める:

- `ActionId`
- `Label`
- `PanelId`
- `Purpose`
- `EnableWhen`
- `InputHint`
- `bEnabled`
- `AvailabilityLabel`
- `Detail`
- `DefaultNameArgument`
- `DefaultStringArgument`
- `bDefaultFlag`
- `Priority`
- `bRecommendedForMinimalLoop`

`CurrentObjectiveAction` では以下を読める:

- `StepOrder`
- `StepLabel`
- `StepStatusLabel`
- `ActionId`
- `ActionLabel`
- `AvailabilityLabel`
- `Detail`
- `DefaultNameArgument`
- `DefaultStringArgument`
- `bDefaultFlag`
- `bHasAction`
- `bEnabled`

`ExecuteHudActionById(ActionId, OptionalNameArgument, OptionalStringArgument, bOptionalFlag)` を使えば、Blueprint 側で `ActionId` ごとの分岐を書かずに主要ボタンを実行できる。

- `grant_developer_card`
  - `OptionalNameArgument` 未指定なら `DefaultDeveloperGrantCardId` を使う
- `build_ai_request`
  - `OptionalStringArgument` を player choice として使う
- `build_fallback_ai`
  - `OptionalStringArgument` を player choice として使う

`VerificationChecks` の各要素では以下を読める:

- `CheckId`
- `Category`
- `Label`
- `bPassed`
- `StatusLabel`
- `Detail`
- `bRequiredForMinimalLoop`

`SessionStatusRows` の各要素では以下を読める:

- `RowId`
- `Label`
- `Value`
- `StatusLabel`
- `Detail`
- `bHealthy`

`Session Status` パネルでは、まず `SessionStatusRows` を先頭に置き、その下に `WalkthroughProgress` と `VerificationChecks` を並べると起動直後の確認が一番早い。

`bAutoRefreshPresentation = true` のままなら、`FocusedEventDisplayName`、`EventActorStatusViewData`、`HudActionStates`、`SessionStatusRows` は探索中も定期更新される。既定の `AutoRefreshIntervalSeconds` は `0.2`。

`HandCardViewData` / `OwnedCardViewData` では以下も読める:

- `KindLabel`
- `StateSummary`
- `DetailSummary`
- `PrimaryActionId`
- `PrimaryActionLabel`
- `PrimaryActionNameArgument`
- `bHasPrimaryAction`
- `bPrimaryActionEnabled`

`EventViewData` では以下も読める:

- `TypeLabel`
- `StatusSummary`
- `DetailSummary`
- `PrimaryActionId`
- `PrimaryActionLabel`
- `PrimaryActionNameArgument`
- `bHasPrimaryAction`
- `bPrimaryActionEnabled`

`EventActorStatusViewData` の各要素では以下を読める:

- `EventId`
- `DisplayName`
- `EventType`
- `PlacementCount`
- `bHasActorPlacement`
- `bAvailable`
- `bCompleted`
- `bIsActive`
- `bIsFocused`
- `bIsInteractableNow`
- `NearestDistance`
- `StatusSummary`

`LastBootstrapDiagnostics.ExpectedEventPlacements` の各要素では以下を読める:

- `EventId`
- `DisplayName`
- `EventType`
- `PlacementCount`
- `bIsPlaced`
- `bIsDuplicate`
- `NextEventIds`

`LastBootstrapDiagnostics.ExpectedEventRoute` の各要素では以下を読める:

- `EventId`
- `DisplayName`
- `EventType`
- `RouteDepth`
- `SuggestedLane`
- `bIsEntryPoint`
- `bIsTerminal`
- `PlacementCount`
- `IncomingEventIds`
- `NextEventIds`

更新通知:

- `OnPresentationUpdated`
- `OnActionResultUpdated`

## 最初の起動確認

1. Widgetから `InitializeNewSession` を呼ぶ
2. `CurrentSnapshot.ActiveEventDisplayName` に最初のイベント名が出る
3. `ResolveActiveEvent` で開始箱を解決する
4. `OwnedCardViewData` に starter cards が出る
5. `EventViewData` に次イベントが解放される
6. イベント地点へ近づくと `FocusedEventDisplayName` に対象名が出る
7. `BuildAiRequestForActiveEvent` で `AllowedRewardCardIds`、`AllowedQuestEventIds`、`PlayerChoice` を含むAI GM入力を確認できる
8. `BuildFallbackAiResponseForActiveEvent` で API 失敗時の固定文面と、許可済みクエスト提案を表示できる
9. `HandCardViewData` の `UnplayableReasons` から、どの制約で使えないかをそのままUI表示できる
10. `LastBootstrapDiagnostics.Issues` から、JSONパスや save slot の不足を起動前に表示できる
11. `LastBootstrapDiagnostics.MissingEventActorIds` を見れば、まだ置いていない `BP_GreeislandEventActor` が分かる
12. `ExpectedEventPlacements` を一覧表示すれば、JSON定義順のマップ配置チェックリストとして使える
13. `ExpectedEventRoute` を一覧表示すれば、Depth ごとの導線メモとしてそのまま使える
14. `EventActorStatusViewData` を一覧表示すれば、配置済みなのに進めないイベントと、進行上は開いているのに置かれていないイベントを切り分けられる
15. `WalkthroughProgress` を一覧表示すれば、次に触るべきMVP手順を HUD 上で自動表示できる
16. `RecommendedHudPanels` を見れば、Session Status / Event Reconciliation / Combat Hand などのパネル分けをそのまま決められる
17. `RecommendedHudActions` を見れば、ボタン名、結びつけるメソッド、置き場所をそのまま決められる
18. `HudActionStates` を使えば、ボタンの enabled/disabled を Blueprint 側で個別実装せずに済む
19. `HudActionButtons` を使えば、definition/state の join なしで actions パネルをそのまま描ける
20. `CurrentObjectiveAction` を最上段に置けば、「いま押すべき一手」を walkthrough と action 状態から自動で出せる
21. `VerificationChecks` を一覧表示すれば、Pass/Needs Work の bring-up レポートをそのまま出せる
22. `StateSummary` / `StatusSummary` をそのまま行表示すれば、ListView の行整形ロジックをかなり減らせる
23. `PrimaryActionId` / `PrimaryActionNameArgument` をそのまま `ExecuteHudActionById` に流せば、行ボタンの押下処理もかなり減らせる
24. `RecommendedHudChecklist` をそのまま表示すれば、Blueprint Widget の最小構成チェックリストとして使える
25. `RecommendedWalkthrough` をそのまま表示すれば、起動後に何をどの順で触るかの確認導線として使える
26. `RecommendedBlueprintAssets` をそのまま表示すれば、作るべき BP アセット一覧として使える

レベル配置ベースで進める場合:

1. `BP_GreeislandBootstrapActor` を1個置く
   - まず `GetBootstrapDiagnostics` を呼び、`Issues` が空であることを確認する
2. `BP_GreeislandEventActor` をイベント地点ごとに置く
   - `MissingEventActorIds` が空になるまで `EventId` を対応づける
3. それぞれの `EventId` を `data/events/events.mvp.json` に対応させる
4. `BP_GreeislandDebugCharacter` でマップを歩き、`E` でイベント地点を起動する
5. 自動発火させたい地点は `BP_GreeislandEventActor` 側で `bAutoTriggerOnOverlap` を有効にする
