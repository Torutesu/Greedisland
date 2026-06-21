#!/usr/bin/env bash
set -euo pipefail

python3 scripts/validate_cards/validate_cards.py data/cards/cards.mvp.json
python3 scripts/validate_events/validate_events.py data/events/events.mvp.json
python3 scripts/validate_events/ue_build_helper_smoke_test.py
python3 scripts/validate_events/ue_editor_preflight_smoke_test.py
python3 scripts/validate_events/ue_path_resolution_smoke_test.py
python3 scripts/validate_events/ue_bootstrap_defaults_smoke_test.py
python3 scripts/validate_events/ue_hud_bootstrap_surface_smoke_test.py
python3 scripts/validate_events/ue_route_graph_smoke_test.py
python3 scripts/validate_events/ue_walkthrough_progress_smoke_test.py
python3 scripts/validate_events/ue_event_actor_status_smoke_test.py
python3 scripts/validate_events/ue_hud_action_state_smoke_test.py
python3 scripts/validate_events/ue_hud_action_button_surface_smoke_test.py
python3 scripts/validate_events/ue_hud_action_executor_smoke_test.py
python3 scripts/validate_events/ue_current_objective_action_smoke_test.py
python3 scripts/validate_events/ue_runtime_viewdata_surface_smoke_test.py
python3 scripts/validate_events/ue_row_action_surface_smoke_test.py
python3 scripts/validate_events/ue_verification_check_smoke_test.py
python3 scripts/validate_events/ue_session_status_rows_smoke_test.py
python3 scripts/validate_cards/rule_smoke_test.py
python3 scripts/validate_cards/combat_smoke_test.py
python3 scripts/validate_cards/playability_reason_smoke_test.py
python3 scripts/validate_cards/exploration_smoke_test.py
python3 scripts/validate_cards/ai_gm_smoke_test.py
python3 scripts/validate_events/ai_fallback_smoke_test.py
python3 scripts/validate_events/zone_flow_smoke_test.py
python3 scripts/validate_events/session_flow_smoke_test.py
python3 scripts/validate_events/mvp_full_run_smoke_test.py
python3 scripts/validate_events/combat_resolution_smoke_test.py
python3 scripts/validate_events/defeat_resolution_smoke_test.py
python3 scripts/validate_events/ai_session_apply_smoke_test.py
python3 scripts/validate_events/developer_grant_smoke_test.py
python3 scripts/validate_events/quest_clear_smoke_test.py
python3 scripts/validate_events/save_restore_smoke_test.py
python3 -m json.tool data/cards/cards.mvp.json >/dev/null
python3 -m json.tool data/ai/ai_gm_schema.json >/dev/null
python3 -m json.tool data/events/events.mvp.json >/dev/null
python3 -m py_compile scripts/validate_cards/*.py
python3 -m py_compile scripts/validate_events/*.py

echo "OK: local checks passed"
