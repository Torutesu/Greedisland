# Card Validation

Validate card data before importing it into Unreal:

```bash
python3 scripts/validate_cards/validate_cards.py data/cards/cards.mvp.json
python3 scripts/validate_cards/rule_smoke_test.py
python3 scripts/validate_cards/combat_smoke_test.py
python3 scripts/validate_cards/exploration_smoke_test.py
python3 scripts/validate_cards/ai_gm_smoke_test.py
```

The script intentionally uses only the Python standard library so it can run on a clean workstation.
