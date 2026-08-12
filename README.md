# Greeisland

カードがゲームのルールを書き換える、AI GM付きカード探索RPGの企画・実装検証リポジトリ。

## Documents

- [プロジェクトブリーフ](docs/00_project_brief.md)
- [要件定義](docs/01_requirements.md)
- [カードシステム設計](docs/02_card_system_design.md)
- [1ゾーンMVP 実装計画](docs/03_mvp_implementation_plan.md)
- [Unreal Engine 5.8 / MCP 検証手順](docs/04_unreal_mcp_validation.md)
- [実装ステータス](docs/05_implementation_status.md)
- [UE5.8 Bring-up Sheet](docs/07_ue_bringup_sheet.md)

## Current Focus

最初のゴールは、UE5.8上で1ゾーン探索、簡易カードバトル、カードによるルール変更、AI GM会話をつないだMVPを作ること。

MVPではオンライン経済、PvP、大規模ワールド、完成版品質の3Dアセットは対象外にする。

Runtime entrypoint:

- [UGreeislandGameSubsystem](/Users/torutano/Documents/Greeisland/UnrealProject/Source/Greeisland/Runtime/GreeislandGameSubsystem.h)
  `BuildUiSnapshot()`、`GetPlayableCombatCardIds()`、`BuildOwnedCardViewData()`、`BuildEventViewData()` でUMG向けの表示データを直接取得できる
- [UGreeislandDebugHudWidget](/Users/torutano/Documents/Greeisland/UnrealProject/Source/Greeisland/UI/GreeislandDebugHudWidget.h)
  Blueprintで見た目を被せるだけで、初期化、イベント解決、戦闘進行、保存復元、近接イベント表示、AI GMリクエスト生成、検証、固定文面フォールバック、開発用カード付与、手札の使用不可理由表示、bootstrap 診断表示、EventActor 配置と進行状態の照合、推奨 HUD パネル定義とレイアウトチェックリスト取得、MVP 一周の walkthrough と進捗取得、verification check 一覧、推奨 Blueprint アセット一覧取得を呼べるデバッグHUDのC++足場
- [AGreeislandDebugHud](/Users/torutano/Documents/Greeisland/UnrealProject/Source/Greeisland/UI/GreeislandDebugHud.h)
  `DebugHudWidgetClass` を指定すると BeginPlay でWidgetをViewportへ載せるHUDクラス
- `DebugHudWidgetClass` が未設定でも、C++ Native HUDへフォールバックしてセッション状態・イベント一覧・カード一覧・AI GM応答を表示する
- Native HUD利用時も `N/O/K/R/C/1-5/Space/F/T` のキーボード操作で、セッション・探索・戦闘・保存・AI GMのMVP導線を進められる
- 自動生成される探索地点にはエンジン標準メッシュとEventIdラベルが付き、レベル用アセットなしでも位置を視認できる
- [AGreeislandDebugGameMode](/Users/torutano/Documents/Greeisland/UnrealProject/Source/Greeisland/GameFramework/GreeislandDebugGameMode.h)
  `PlayerController` と `HUD` に加えて、`DefaultPawnClass` をデバッグ用キャラクターへ接続し、未配置のMVP Bootstrap/EventActor/地面をBeginPlayで補完するGameMode足場
- [AGreeislandDebugCharacter](/Users/torutano/Documents/Greeisland/UnrealProject/Source/Greeisland/Characters/GreeislandDebugCharacter.h)
  `WASD` 移動、マウス視点、`E` インタラクトで探索イベントを踏める最小プレイアブルキャラクター
- [AGreeislandBootstrapActor](/Users/torutano/Documents/Greeisland/UnrealProject/Source/Greeisland/Actors/GreeislandBootstrapActor.h)
  BeginPlay時にセッション初期化または復元を自動実行するレベル配置用Actor。診断で `EventActor` の配置漏れや重複を拾え、想定イベント順のチェックリストと導線グラフも返せる
- [AGreeislandEventActor](/Users/torutano/Documents/Greeisland/UnrealProject/Source/Greeisland/Actors/GreeislandEventActor.h)
  `EventId` を持ち、探索イベントまたは戦闘開始をSubsystemへ委譲するレベル配置用Actor。オーバーラップ自動発火と近接インタラクトにも対応
- [UE5.8 デバッグHUD接続手順](/Users/torutano/Documents/Greeisland/docs/06_ue_debug_hud_setup.md)
- [UE5.8 Bring-up Sheet](/Users/torutano/Documents/Greeisland/docs/07_ue_bringup_sheet.md)

Project config:

- [DefaultGame.ini](/Users/torutano/Documents/Greeisland/UnrealProject/Config/DefaultGame.ini)
  C++の `AGreeislandDebugGameMode` をデフォルトGameModeに設定
  `CardJsonPath` / `EventJsonPath` の既定値は `ProjectDir` 基準で `../data/...` に調整済み
- [DefaultEngine.ini](/Users/torutano/Documents/Greeisland/UnrealProject/Config/DefaultEngine.ini)
  UE標準の `/Engine/Maps/Entry` をEditor/ゲームの既定マップに設定し、起動時にC++のMVPゾーン生成へ接続
- [DefaultInput.ini](/Users/torutano/Documents/Greeisland/UnrealProject/Config/DefaultInput.ini)
  `WASD` / マウス / `E` に加えて、Native HUD用のセッション・戦闘・保存・AI GMショートカットを設定
- [UGreeislandProjectSettings](/Users/torutano/Documents/Greeisland/UnrealProject/Source/Greeisland/Runtime/GreeislandProjectSettings.h)
  `Project Settings > Game > Greeisland` から JSON パスや保存スロットなどを調整できる

## Validation

```bash
./scripts/run_local_checks.sh
```

Individual checks:

```bash
python3 scripts/validate_cards/validate_cards.py data/cards/cards.mvp.json
python3 scripts/validate_events/validate_events.py data/events/events.mvp.json
python3 scripts/validate_cards/rule_smoke_test.py
python3 scripts/validate_cards/combat_smoke_test.py
python3 scripts/validate_cards/exploration_smoke_test.py
python3 scripts/validate_cards/ai_gm_smoke_test.py
python3 scripts/validate_events/zone_flow_smoke_test.py
python3 scripts/validate_events/session_flow_smoke_test.py
python3 scripts/validate_events/ue_route_graph_smoke_test.py
python3 scripts/validate_events/ue_walkthrough_progress_smoke_test.py
python3 scripts/validate_events/ue_event_actor_status_smoke_test.py
python3 scripts/validate_events/ue_hud_action_state_smoke_test.py
python3 scripts/validate_events/ue_runtime_viewdata_surface_smoke_test.py
python3 scripts/validate_events/ue_verification_check_smoke_test.py
python3 scripts/validate_events/combat_resolution_smoke_test.py
python3 scripts/validate_events/defeat_resolution_smoke_test.py
python3 scripts/validate_events/ai_session_apply_smoke_test.py
python3 scripts/validate_events/quest_clear_smoke_test.py
python3 scripts/validate_events/save_restore_smoke_test.py
```
