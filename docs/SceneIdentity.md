# Scene cube identity

`CubeId` is a scene-local unsigned 64-bit integer. Zero (`invalidCubeId`)
means no cube identity. Standalone `GameObject`s and the player default to zero.
`addCube()` still returns `GameObject&`; its trailing aggregate `id` field is
assigned by Scene, so existing object initializers keep their meaning.

Normal creation allocates increasing IDs starting at one. Deletion never rolls
back the allocation watermark. Exhausting the integer range throws
`std::overflow_error`, including after deleting the final allocated ID.
Names may repeat and play no role in identity.

`findCube(id)` returns a mutable or const pointer, or null for invalid/missing
IDs. `removeCube(id)` returns true if deleted and false otherwise. Both scan
the cube deque. Deleting a cube immediately removes it from subsequent render
and collision candidate enumeration.

Editor selection must store an ID and resolve it when needed. Appending cubes
preserves references, but arbitrary deque deletion can invalidate references
and pointers to surviving cubes as well. Do not retain these across lifecycle
operations or dereference a removed object. Collision candidates remain borrowed
only for a movement call; do not mutate the scene during that call.

Scene copy construction and copy assignment copy cube IDs and the allocation
watermark, including retired IDs, along with independently owned object data.
Each copy is its own identity namespace and can edit/delete/create independently.
The same number in different scenes need not identify the same object. Assignment
replaces the destination scene namespace; external selections must be reset.

Future loading can build a fresh Scene using `restoreCube(id, name, ...)` in
ascending ID order, then set remaining properties such as colliders on the
returned object. Restoration rejects zero and IDs below the current watermark
with `std::invalid_argument`; it advances allocation past the restored ID and
handles the maximum ID without wrapping. Sort and validate saved IDs before
restoring into a temporary scene. Serialization itself is outside this task.

For compatibility, `Scene::cubes` and GameObject fields remain public. Treat
`id` as Scene-managed and use Scene methods for structural changes. Direct
container insertion, replacement, or ID assignment bypasses these guarantees.
