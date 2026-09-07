# Authored scenes: JSON version 1

The Hierarchy panel provides Scene path, Save, Load, and persistent success/error
feedback. Paths are UTF-8, may be absolute, and otherwise resolve from the process
working directory. The default is scene.json. No unsaved-change prompt is shown.

All fields below are required. Additional fields are ignored (not retained);
duplicate JSON keys are rejected. The document must be one complete JSON value.

```json
{
  "version": 1,
  "nextCubeId": "2",
  "player": {
    "name": "Player",
    "position": [0, -0.25, 0],
    "scale": [0.5, 0.75, 0.5],
    "color": [1, 0.58, 0.12],
    "collider": {"enabled": true, "offset": [0, 0, 0], "halfExtents": [1, 1, 1]},
    "movementSpeed": 4
  },
  "cubes": [
    {"id": "1", "name": "Cube", "position": [3, 0, 0],
     "scale": [1, 1, 1], "color": [0.12, 0.78, 0.90], "collider": null}
  ]
}
```

## Field mapping and validation

| JSON | Engine field and rules |
| --- | --- |
| version | Integer 1 only; no migration |
| nextCubeId | Scene allocation watermark, canonical uint64 decimal string; greater than every live ID, or "0" for exhausted allocation |
| cubes | Scene::cubes in authored array order, including arbitrary ID order |
| cubes[].id | GameObject::id; unique canonical decimal string in 1..18446744073709551615; no leading zeros |
| name | GameObject::name, UTF-8 string; empty and duplicate names allowed |
| position | GameObject::position; three finite float components |
| scale | GameObject::scale; three positive finite float components, mesh half dimensions |
| color | GameObject::color; three finite float components in [0,1] |
| collider | GameObject::boxCollider; null means absent; object means attached, even when disabled |
| collider.enabled | BoxCollider::enabled, JSON boolean |
| collider.offset | BoxCollider::offset, three finite float components |
| collider.halfExtents | BoxCollider::halfExtents, three positive finite float components |
| player | Player::object fields above, without cube ID |
| player.movementSpeed | Player::movementSpeed, finite float; negative values preserved because existing movement clamps them to zero |

Player position is the authored spawn/current transform. There is no separate
spawn field, rotation, gravity, velocity, or additional authored Player setting
in the current model. The player remains a standalone object with ID zero.
Camera/navigation state, selection, widget buffers, pointers, OpenGL resources,
collision results, and recovery/contact state are not saved.

Numbers outside float range or nonzero numbers that underflow to zero are
rejected. Normal JSON decimals round to float on load; values saved from engine
floats round-trip exactly. Both enabled and disabled collider configurations
must pass the existing worldAabb validation, including local and scaled bound
overflow checks. IDs use strings to avoid precision loss through JSON tools
that use IEEE-754 doubles. Persisting the allocation watermark also prevents
reuse of IDs deleted before saving; exhaustion never wraps.

## Transactions and limitations

ScenePersistence is independent of ImGui. Load parses and validates a temporary
Scene and commits with nonthrowing swaps. Failure leaves all active scene data
and its allocator unchanged. Editor Operations::load clears selection only on
success; the widget action also clears validation feedback, active widget state,
and navigation capture. The renderer reads the replacement scene that frame.

Save validates and encodes before writing, exclusively creates a unique sibling
temporary file, checks writes, flushes and closes it, then uses Windows
MoveFileExW(REPLACE_EXISTING | WRITE_THROUGH). It never truncates the destination.
Failure before replacement leaves the previous destination intact; normal failure
paths clean up the temporary file. Missing parent directories are reported, not
created. Locked/read-only destinations and other filesystem errors are visible.
This is a local filesystem replacement strategy, not a guarantee against power
loss or unusual network filesystem semantics. A process crash may leave a sibling
temporary file. Files are loaded into memory; there is no streaming/size quota.

The pinned MIT nlohmann/json dependency and hash are documented in
third_party/json/README.vendor.md. Builds need no dependency download.

## Verification

The scene_persistence test uses isolated temporary files and covers all authored
fields, duplicate names, arbitrary cube order, absent/disabled/enabled colliders,
IDs above 2^53 and at uint64 maximum, deleted-ID continuation, invalid schema/data,
failed-load scene/selection preservation, missing paths, invalid saves, locked
destination replacement failure, temporary cleanup, and successful replacement.

For an interactive restart check: edit cubes, save to an absolute path, close
and restart sandbox, then load that path and inspect the objects. Also try
malformed JSON and a destination you cannot write. Automated checks do not
substitute for this manual interaction.
