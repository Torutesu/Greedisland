# カードシステム設計

最終更新: 2026-06-21

## 1. 設計方針

カードは「効果」ではなく「ルール変更の単位」として扱う。MVPでは自由記述スクリプトを許さず、C++側に実装済みの効果タイプ、制約タイプ、ルールチャンネルをカードデータから組み合わせる。

目的:

- Claude Codeでレビュー・編集しやすい。
- デバッグ可能な決定的ルール解決にする。
- 将来AI GMがカードやクエストを提案しても、ゲーム状態は壊れない。
- カード同士の抜け穴やコンボを、型付きデータとして表現できる。

## 2. カード分類

### 2.1 Action

即時効果を持つカード。

例:

- 敵にダメージを与える。
- HPを回復する。
- 次の探索イベントをスキップする。
- 手札を引く。

### 2.2 Item

所持または使用で効果を持つカード。

例:

- 所持している間、探索報酬が増える。
- 1回だけイベント失敗を無効化する。
- NPC交渉時の価格を下げる。

### 2.3 Rule

ゲームルールを変更するカード。

例:

- 1ターンの移動回数を増やす。
- 戦闘中のみ使えるカードを探索中にも使えるようにする。
- 報酬抽選時のレアリティ補正を変更する。

### 2.4 Constraint

使用条件またはゲーム状態に制約を加えるカード。

例:

- 4人パーティ扱いでないと使用できない。
- 手札が3枚以下のときのみ有効。
- 次の2ターン、攻撃カードを使えない代わりに報酬が増える。

### 2.5 Key

進行やクリア条件に関係するカード。

例:

- 1ゾーンMVPのクリアに必要な証。
- 特定NPCとの会話を解放する。
- ボスイベントを起動する。

## 3. データモデル案

MVPでは JSON を原本にし、UE側で `UDataTable` または `UPrimaryDataAsset` へインポートする方式を推奨する。理由は、カード定義をGitで差分確認でき、Claude Codeでも直接編集しやすいため。

### 3.1 CardDefinition

```json
{
  "cardId": "rule_phase_bridge_001",
  "displayName": "境界線の通行証",
  "kind": "Rule",
  "rarity": "Rare",
  "tags": ["phase", "exploration", "exception"],
  "playablePhases": ["Exploration"],
  "cost": {
    "energy": 1
  },
  "constraints": [
    {
      "type": "HasTagInCollection",
      "tag": "contract",
      "minCount": 1
    }
  ],
  "effects": [
    {
      "type": "ModifyRule",
      "channel": "CardPlayablePhase",
      "operation": "AddPhase",
      "targetTag": "battle_only",
      "phase": "Exploration",
      "duration": {
        "type": "Turns",
        "value": 1
      },
      "priority": 50
    }
  ],
  "flavorText": "町と戦場の境目を一歩だけ曖昧にする。"
}
```

### 3.2 C++構造体イメージ

```cpp
UENUM(BlueprintType)
enum class ECardKind : uint8
{
    Action,
    Item,
    Rule,
    Constraint,
    Key
};

UENUM(BlueprintType)
enum class ERuleChannel : uint8
{
    DrawCount,
    HandLimit,
    MoveCount,
    EncounterDifficulty,
    TradePrice,
    RewardMultiplier,
    CardPlayablePhase,
    PartyRequirement,
    TargetingRule
};

USTRUCT(BlueprintType)
struct FCardDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName CardId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ECardKind Kind;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FGameplayTag> Tags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FCardConstraintSpec> Constraints;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FCardEffectSpec> Effects;
};
```

## 4. ルール解決パイプライン

1. Request
   - プレイヤーがカード使用、移動、交渉、報酬受取などの行動を要求する。

2. Context Build
   - 現在フェーズ、手札、所持カード、ターン数、イベント状態、AI提案内容などを集める。

3. Constraint Check
   - カード固有制約、グローバル制約、Ruleカードによる例外を判定する。

4. Rule Patch Collection
   - 有効なRule/Constraintカードから、対象チャンネルに関係するルール変更を集める。

5. Priority Resolve
   - priority, duration, source, timestamp の順に競合を解決する。

6. Effect Resolve
   - 即時効果、持続効果、報酬、状態変更を処理する。

7. Commit
   - 検証済みの状態変更だけをゲーム状態に反映する。

8. Log
   - 何がどの順序で適用されたかをデバッグログに残す。

## 5. 制約カードの扱い

制約カードは単なる使用条件ではなく、プレイヤーが攻略に使えるルール部品として扱う。

例:

- 「単独行動」: パーティ人数を1人として扱う。報酬倍率 +1、ただし協力カード不可。
- 「仮契約」: 3ターンだけNPCをパーティ人数に数える。
- 「夜間限定」: 夜フェーズでのみ使用可。別カードで探索フェーズを夜扱いにできる。
- 「沈黙の誓い」: 会話カード不可。代わりにAI GM交渉で威圧選択肢が出る。

## 6. コンボ設計の最小セット

MVPでは以下3つのコンボを実装候補にする。

### 6.1 パーティ人数ハック

- Constraint: 「4人パーティでのみ使用可」
- Item: 「仮契約書」 NPCを一時的にパーティ人数へ加算
- Rule: 「代理参加」 召喚/契約タグを人数条件に含める

狙い:

本来使えない強力カードを、カード解釈で使用可能にする。

### 6.2 フェーズ境界ハック

- Action: 「戦闘中のみ使用可」の報酬奪取カード
- Rule: 「境界線の通行証」 battle_onlyタグを探索中にも一時許可
- Event: 探索イベントを擬似戦闘として扱う

狙い:

探索と戦闘の境界をカードで曖昧にする。

### 6.3 報酬倍率ハック

- Constraint: 次ターン攻撃不可
- Rule: 報酬倍率 +2
- Item: 攻撃以外の勝利条件を追加

狙い:

リスクをカードで踏み倒すのではなく、別勝利条件に変換する。

## 7. AI GMとの境界

AI GMはカード効果を直接実行しない。AI GMは以下のような構造化提案を返す。

```json
{
  "speakerName": "門番リオ",
  "dialogue": "通行証がないなら、別の証明を見せてもらおう。",
  "intent": "negotiation",
  "proposedQuestId": "quest_gate_token",
  "allowedRewardCardIds": ["key_gate_token_001"],
  "difficultyHint": "medium"
}
```

ゲーム側は以下を検証する。

- speakerNameが現在イベントに存在するNPCか
- proposedQuestIdが許可されたクエストか
- allowedRewardCardIdsが定義済みカードか
- 報酬数、難易度、分岐が許容範囲内か
- 禁止表現やIP類似表現がないか

## 8. 初期カードリスト案

MVP初期の15枚:

| ID | 種別 | 役割 |
|---|---|---|
| act_strike_001 | Action | 基本攻撃 |
| act_guard_001 | Action | 防御 |
| act_patch_heal_001 | Action | 回復 |
| act_draw_001 | Action | ドロー |
| act_battle_claim_001 | Action | 戦闘中のみ報酬奪取 |
| item_contract_001 | Item | NPC仮契約 |
| item_lucky_compass_001 | Item | 探索報酬補正 |
| item_null_charm_001 | Item | 失敗1回無効 |
| rule_phase_bridge_001 | Rule | 戦闘専用カードを探索中に許可 |
| rule_party_proxy_001 | Rule | 契約タグを人数条件に含める |
| rule_reward_oath_001 | Rule | 制約中の報酬倍率上昇 |
| rule_move_plus_001 | Rule | 移動回数 +1 |
| con_four_party_001 | Constraint | 4人パーティ条件 |
| con_silent_oath_001 | Constraint | 会話不可、報酬上昇 |
| key_zone_core_001 | Key | 1ゾーンクリア証 |

