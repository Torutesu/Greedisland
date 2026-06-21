# Event Validation

Validate MVP zone event data:

```bash
python3 scripts/validate_events/validate_events.py data/events/events.mvp.json
python3 scripts/validate_events/zone_flow_smoke_test.py
python3 scripts/validate_events/session_flow_smoke_test.py
python3 scripts/validate_events/combat_resolution_smoke_test.py
python3 scripts/validate_events/defeat_resolution_smoke_test.py
python3 scripts/validate_events/ai_session_apply_smoke_test.py
python3 scripts/validate_events/quest_clear_smoke_test.py
python3 scripts/validate_events/save_restore_smoke_test.py
```
