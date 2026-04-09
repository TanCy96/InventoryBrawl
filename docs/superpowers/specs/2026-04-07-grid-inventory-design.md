# Grid Inventory Design

**Goal:** Define a generic, data-driven Unreal C++ inventory system that supports irregular non-hollow item footprints, rotation, drag/drop movement, discard, and atomic transfer between two inventories.

**Project context:** `InventoryBrawl` is currently a small single-module Unreal project with minimal native gameplay code. There is no existing inventory framework, replication layer, or save/load system to integrate with, so the design should favor a clean reusable gameplay core that can be attached to any actor and driven by UMG.

## Scope

### In scope for v1
- Generic inventory ownership through a reusable actor component
- Inventory definitions expressed as Unreal primary data assets
- Grid-based placement with arbitrary connected occupied-cell masks
- Item rotation in 90-degree increments
- Drag/drop movement within the same inventory
- Discard/remove support
- Atomic transfer from inventory A to inventory B
- C++ validation and authoritative state mutation APIs
- UMG-driven interaction and placement preview
- Automated C++ tests for shape validation and grid logic

### Explicitly out of scope for v1
- Equipment slots
- World pickup spawning beyond a discard hook/event
- Save/load persistence
- Network replication
- Crafting, item durability, or use-effect gameplay
- Hollow or disconnected item footprints

## Recommended architecture

### `UInventoryItemDefinition`
A `UPrimaryDataAsset` defines the static item data. At minimum each definition should contain:
- Display name
- Optional icon/visual metadata for UI
- Canonical occupied-cell mask in local item space
- Optional stacking settings if added later

The shape mask is authored once in editor data and treated as the source of truth. Runtime logic derives all occupied cells from this canonical shape plus rotation.

### Runtime item record
Each inventory stores lightweight runtime entries rather than spawning item actors. A runtime item record should contain:
- Stable runtime item id
- Reference to `UInventoryItemDefinition`
- Anchor cell in container-local grid space
- Rotation state (`0`, `90`, `180`, `270`)
- Stack count only if the item definition allows stacking later

Runtime entries do not duplicate rotated occupied cells. Occupied cells are computed on demand from the definition plus rotation.

### `UInventoryComponent`
`UInventoryComponent` is the reusable owner-facing API attached to any actor that can hold items, including the player and other containers. It owns:
- Grid dimensions
- Runtime item records
- Query APIs for UI and gameplay
- Mutation APIs for add, move, rotate, transfer, and discard
- Delegates/events for UI refresh and gameplay hooks

This keeps ownership generic and avoids baking inventory rules into a specific pawn or widget.

### Grid math helper
A dedicated pure C++ helper should own placement math so it can be tested independently from actor/component behavior. It is responsible for:
- Validating item definitions are connected and non-hollow according to v1 rules
- Rotating occupied-cell masks
- Converting a runtime item into occupied grid cells
- Bounds checks
- Overlap checks
- Same-inventory move checks
- Cross-inventory transfer validation

This helper should have no UI dependencies and minimal Unreal object coupling.

### UMG integration
UMG widgets handle:
- Drag input
- Hover preview
- Rotation requests while dragging
- Displaying valid or invalid target feedback

Widgets do not mutate inventory state directly. Every commit goes through `UInventoryComponent` APIs, with C++ as the authority for validation and mutation.

## Data model

### Item shape representation
Irregular items are represented as a set of occupied local cells, for example:
- `[(0,0), (1,0), (0,1)]` for an L-shape
- `[(0,0), (1,0), (2,0), (1,1)]` for a T-shape

Constraints for v1:
- At least one occupied cell
- All cells are unique
- The set must be 4-neighbor connected
- Hollow shapes are rejected

The definition format should allow future widening if you later want more permissive shapes, but v1 validation should enforce the above rules.

### Grid coordinates
Each inventory grid is addressed in integer cell coordinates with a top-left origin:
- `X` increases to the right
- `Y` increases downward

Each runtime item stores one anchor cell. The anchor is the origin used to place the rotated local mask into container space.

### Rotation
Rotation is limited to 90-degree clockwise steps. Rotating an item means:
- Transform the local occupied cells
- Normalize the rotated shape so it remains anchored predictably
- Re-run fit validation against the target inventory

If a rotate request is issued in place and the rotated result cannot fit at the current anchor, the request fails without mutating state.

## Core behaviors

### Placement
Placement succeeds only when every occupied cell:
- Is inside the target inventory bounds
- Does not overlap another item, except the currently moved item during same-inventory moves

The API should return a structured result rather than a bool so UI can distinguish:
- Success
- Out of bounds
- Overlap
- Invalid definition
- Unsupported operation

### Move within one inventory
Dragging an item inside the same container is implemented as a validated reposition operation. The source item remains the same runtime record; only its anchor and possibly rotation change on success.

### Transfer between inventories
Moving from inventory A to inventory B is treated as a single transaction:
1. Validate the target placement in inventory B
2. If valid, remove from A and add to B in one authoritative operation
3. If invalid, leave the item unchanged in A

This avoids item loss or duplication during UI drag/drop.

### Discard
Discard removes the runtime item from its owning inventory and broadcasts an event/delegate containing the definition and last-known item state. Gameplay can later bind this to spawn a world pickup, but that spawn behavior is outside v1 scope.

## API direction

The exact signatures can evolve during implementation, but the system should expose a C++ API in this shape:
- Query all runtime items in an inventory
- Query occupied cells for a runtime item
- Test placement preview at a target anchor and rotation
- Try add item from a definition
- Try move existing item within the same inventory
- Try rotate existing item
- Try transfer item to another inventory
- Remove or discard item by runtime id

The key design constraint is that all mutation paths share the same placement rules instead of duplicating validation in separate functions.

## Error handling and validation

### Definition validation
Item definitions should be validated as early as practical, ideally both:
- In editor-facing validation paths where possible
- At runtime before an invalid definition is admitted into inventory state

An invalid definition should fail loudly with a useful log message and a structured placement/add result.

### Mutation failures
Invalid operations never partially mutate state. Every failure leaves the inventory unchanged.

This is especially important for:
- Rotate in place
- Transfer between two inventories
- Drag/drop commits after preview

## Testing strategy

### Automated C++ tests
Automated tests should cover:
- Connected-shape acceptance
- Rejection of empty, duplicate, disconnected, and hollow shapes
- 90-degree rotation transforms
- Placement within bounds
- Rejection on overlap
- Same-inventory movement rules
- Atomic transfer between two inventories
- Discard removing the runtime item cleanly

The test surface should focus on the grid helper and inventory component APIs rather than UMG interaction details.

### Manual verification
Manual editor verification should confirm:
- Dragging inside one inventory
- Rotating while dragging
- Rejecting invalid drops with clear UI feedback
- Discarding from the inventory UI
- Moving an item from one visible inventory widget to another

## Unreal integration notes

- Keep the gameplay core in the existing `InventoryBrawl` module rather than introducing another module in v1.
- Prefer smaller focused files because the project is still early and easy to keep organized.
- Use Unreal-friendly types and reflection only where needed for editor/UI exposure; keep grid math as plain C++ where practical.
- Let Blueprint/UMG consume delegates and callable methods, but avoid putting authoritative rules in Blueprint.

## Expected first implementation slices

The work should likely break into these phases:
1. Item definition asset type and shape validation
2. Pure grid math helper
3. Inventory runtime data structures and component API
4. Transfer/discard flows
5. UMG widget integration
6. Automated tests and manual verification pass

## Success criteria

The v1 inventory is successful when:
- Any actor can own an inventory by adding `UInventoryComponent`
- Items are defined through `UPrimaryDataAsset`
- Irregular connected non-hollow item masks can be placed and rotated
- A player-facing UMG inventory can drag, rotate, discard, and move items between two containers
- All authoritative state changes are validated in C++
- Automated tests cover the core grid and transfer logic
