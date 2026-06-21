# UE5.8 デバッグHUD接続手順

最終更新: 2026-06-21

## 目的

このリポジトリのC++ロジックを、UE5.8上で最短で触れるようにする。

## すでに用意されているC++クラス

- `UGreeislandGameSubsystem`
  - ゲーム進行、保存復元、AI反映、UI向けスナップショット
- `UGreeislandDebugHudWidget`
  - Blueprintで見た目を被せる前提の `UUserWidget`
- `AGreeislandDebugHud`
  - `DebugHudWidgetClass` を生成してViewportへ追加する `AHUD`
- `AGreeislandDebugGameMode`
  - `DefaultPawnClass`、`PlayerControllerClass`、`HUDClass` をデバッグ向けに設定した `AGameModeBase`
- `AGreeislandDebugPlayerController`
  - マウス表示とクリックを有効にした `APlayerController`
- `AGreeislandDebugCharacter`
  - `WASD` 移動、マウス視点、`E` インタラクトを持つ最小プレイアブルキャラクター

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

## Blueprintで使うデータ

`UGreeislandDebugHudWidget` から参照できるもの:

- `CurrentSnapshot`
- `PlayableCombatCardIds`
- `OwnedCardViewData`
- `HandCardViewData`
- `EventViewData`
- `LastActionResult`

更新通知:

- `OnPresentationUpdated`
- `OnActionResultUpdated`

## 最初の起動確認

1. Widgetから `InitializeNewSession` を呼ぶ
2. `CurrentSnapshot.ActiveEventDisplayName` に最初のイベント名が出る
3. `ResolveActiveEvent` で開始箱を解決する
4. `OwnedCardViewData` に starter cards が出る
5. `EventViewData` に次イベントが解放される

レベル配置ベースで進める場合:

1. `BP_GreeislandBootstrapActor` を1個置く
2. `BP_GreeislandEventActor` をイベント地点ごとに置く
3. それぞれの `EventId` を `data/events/events.mvp.json` に対応させる
4. `BP_GreeislandDebugCharacter` でマップを歩き、`E` でイベント地点を起動する
5. 自動発火させたい地点は `BP_GreeislandEventActor` 側で `bAutoTriggerOnOverlap` を有効にする
