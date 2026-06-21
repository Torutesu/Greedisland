# 実装ステータス

最終更新: 2026-06-21

## 現在の実装範囲

### 完了

- UE5.8向けC++プロジェクトの最小構成
- MVPカード15枚のJSON原本
- AI GMレスポンス用JSON Schema
- カードJSON検証スクリプト
- MVPルールハックのスモークテスト
- 簡易戦闘ループのC++実装
- 簡易戦闘ループのスモークテスト
- 探索イベント解決/カード報酬付与のC++実装
- 探索イベント解決のスモークテスト
- AI GMレスポンス検証器のC++実装
- AI GMレスポンス検証のスモークテスト
- 1ゾーンMVPイベントJSON
- 探索イベントJSONローダーのC++実装
- イベント定義検証とゾーン導線スモークテスト
- セッション初期化/進行/戦闘開始/AIリクエスト生成のC++実装
- セッション進行スモークテスト
- Session <-> SaveGame 変換のC++実装
- 保存復元スモークテスト
- セッション戦闘進行と勝利時報酬確定のC++実装
- 戦闘解決スモークテスト
- 敗北時のセッション復帰処理
- AI GMレスポンスをセッション報酬へ反映する処理
- 敗北復帰とAI反映のスモークテスト
- クエスト状態同期とゾーンクリア判定
- クエスト/クリア/保存復元の更新テスト
- 1ゾーンMVP一周の統合スモークテスト
- UE起動向けJSONパス解決チェック
- BootstrapActor と Project Settings の既定値同期チェック
- HUD向け bootstrap diagnostics 表示補助
- EventActor 配置漏れ / 重複の bootstrap diagnostics
- Event placement checklist DTO
- Event route checklist DTO
- HUD layout checklist DTO
- HUD panel definition DTO
- HUD action definition DTO
- HUD action state DTO
- HUD walkthrough checklist DTO
- HUD walkthrough progress DTO
- HUD event actor status DTO
- Blueprint asset checklist DTO
- Runtime card/event summary strings for UI rows
- GameInstanceSubsystem ベースのランタイム接続層
- UMG向け UI snapshot / playable card 取得API
- Blueprint向け card/event view data 取得API
- Blueprint被せ前提のデバッグHUD UUserWidget 足場
- HUD向け focused event 表示 / 近接インタラクト補助
- HUD向け AI GM request 生成補助
- HUD向け AI GM validation / fallback response 補助
- 開発者用カード付与コマンド
- 手札カードの使用不可理由と有効パーティ人数表示
- ViewportへデバッグHUDを自動表示するAHUD足場
- Debug PlayerController / Debug GameMode とUE接続手順
- Debug Character と最小入力マッピング
- UnrealProject/Config の初期GameMode設定
- DeveloperSettings 経由のDebug HUD設定
- レベル配置用 BootstrapActor / EventActor
- EventActor の近接インタラクト / オーバーラップ発火補助
- Unreal C++カード型定義
- Unreal C++カードJSONローダー
- Unreal C++ルール解決器 v0
- 戦闘、探索、AI GM、セーブの初期DTO

### 検証済み

```bash
./scripts/run_local_checks.sh
```

結果:

```text
OK: local checks passed
```

```bash
python3 scripts/validate_cards/validate_cards.py data/cards/cards.mvp.json
```

結果:

```text
OK: 15 cards validated
```

```bash
python3 scripts/validate_events/validate_events.py data/events/events.mvp.json
```

結果:

```text
OK: 5 events validated
```

```bash
python3 scripts/validate_cards/rule_smoke_test.py
```

結果:

```text
OK: rule smoke tests passed
```

```bash
python3 scripts/validate_cards/combat_smoke_test.py
```

結果:

```text
OK: combat smoke tests passed
```

```bash
python3 scripts/validate_cards/exploration_smoke_test.py
```

結果:

```text
OK: exploration smoke tests passed
```

```bash
python3 scripts/validate_cards/ai_gm_smoke_test.py
```

結果:

```text
OK: AI GM smoke tests passed
```

```bash
python3 scripts/validate_events/zone_flow_smoke_test.py
```

結果:

```text
OK: zone flow smoke tests passed
```

```bash
python3 scripts/validate_events/session_flow_smoke_test.py
```

結果:

```text
OK: session flow smoke tests passed
```

```bash
python3 scripts/validate_events/mvp_full_run_smoke_test.py
```

結果:

```text
OK: MVP full run smoke tests passed
```

```bash
python3 scripts/validate_events/ue_path_resolution_smoke_test.py
```

結果:

```text
OK: UE path resolution smoke tests passed
```

```bash
python3 scripts/validate_events/ue_bootstrap_defaults_smoke_test.py
```

結果:

```text
OK: UE bootstrap defaults smoke tests passed
```

```bash
python3 scripts/validate_events/ue_route_graph_smoke_test.py
```

結果:

```text
OK: UE route graph smoke tests passed
```

```bash
python3 scripts/validate_events/ue_walkthrough_progress_smoke_test.py
```

結果:

```text
OK: UE walkthrough progress smoke tests passed
```

```bash
python3 scripts/validate_events/ue_event_actor_status_smoke_test.py
```

結果:

```text
OK: UE event actor status smoke tests passed
```

```bash
python3 scripts/validate_events/ue_hud_action_state_smoke_test.py
```

結果:

```text
OK: UE HUD action state smoke tests passed
```

```bash
python3 scripts/validate_events/ue_runtime_viewdata_surface_smoke_test.py
```

結果:

```text
OK: UE runtime viewdata surface smoke tests passed
```

```bash
python3 scripts/validate_events/combat_resolution_smoke_test.py
```

結果:

```text
OK: combat resolution smoke tests passed
```

```bash
python3 scripts/validate_events/defeat_resolution_smoke_test.py
```

結果:

```text
OK: defeat resolution smoke tests passed
```

```bash
python3 scripts/validate_events/ai_session_apply_smoke_test.py
```

結果:

```text
OK: AI session apply smoke tests passed
```

```bash
python3 scripts/validate_events/quest_clear_smoke_test.py
```

結果:

```text
OK: quest clear smoke tests passed
```

```bash
python3 scripts/validate_events/save_restore_smoke_test.py
```

結果:

```text
OK: save restore smoke tests passed
```

```bash
python3 -m json.tool data/cards/cards.mvp.json >/dev/null
python3 -m json.tool data/ai/ai_gm_schema.json >/dev/null
```

結果:

```text
JSON OK
```

## 未検証

- UE5.8エディタでのプロジェクト起動
- UnrealHeaderTool実行
- C++コンパイル
- UMG表示
- 実機でのカードロード
- 実機での Bring-up Sheet なぞり確認

理由:

このMac上では `/Applications` と `/Users/Shared/Epic Games` にUE5.8またはUnrealEditorが見つからなかったため。

## 次の実装順

1. UE5.8をインストールした環境で `UnrealProject/Greeisland.uproject` を開く。
2. UnrealHeaderTool / C++ビルドのエラーを修正する。
3. `data/cards/cards.mvp.json` をUEプロジェクトからロードできる配置にコピーまたは参照する。
4. `URuleResolver::CanPlayCard` を開発用Widgetまたは自動テストから呼び出す。
5. パーティ人数ハック、フェーズ境界ハック、報酬倍率ハックの3ケースをテストする。
6. 戦闘ループの最小実装に進む。
