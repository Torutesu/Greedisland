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
  - `PlayerControllerClass` と `HUDClass` をデバッグ向けに設定した `AGameModeBase`
- `AGreeislandDebugPlayerController`
  - マウス表示とクリックを有効にした `APlayerController`

## UE側で作るもの

1. `BP_GreeislandDebugHudWidget`
   - Parent Class: `UGreeislandDebugHudWidget`

2. `BP_GreeislandDebugHud`
   - Parent Class: `AGreeislandDebugHud`
   - `DebugHudWidgetClass` に `BP_GreeislandDebugHudWidget` を設定

3. `BP_GreeislandDebugGameMode`
   - Parent Class: `AGreeislandDebugGameMode`
   - 必要なら `HUD Class` を `BP_GreeislandDebugHud` に差し替え

## World Settings / Project Settings

最短では、対象マップの `World Settings` で `GameMode Override` を `BP_GreeislandDebugGameMode` にする。

常時使いたい場合は `Project Settings > Maps & Modes` で Default GameMode に設定する。

補足:

- `UnrealProject/Config/DefaultGame.ini` では C++ の `AGreeislandDebugGameMode` が `GlobalDefaultGameMode` に設定済み
- Blueprint版を使う場合は `Project Settings` または `World Settings` で `BP_GreeislandDebugGameMode` へ差し替える

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
