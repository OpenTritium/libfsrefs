# ReFS v1 to v3 Delta Checklist

## Baseline

- Repository: `libyal/libfsrefs`
- Local baseline commit: `ff13b26839b9428341814b9624ef528536ae1ca8`
- Goal of this note: capture the current parser flow, the existing v3-aware code, and the remaining v1-centric assumptions that block reliable ReFS v3 file-index enumeration.

## Current Open Path

The current open path is:

1. `libfsrefs_volume_open_read()` in `libfsrefs_volume.c`
2. `libfsrefs_volume_header_read_file_io_handle()`
3. `libfsrefs_file_system_read_superblock()`
4. `libfsrefs_file_system_read_checkpoints()`
5. `libfsrefs_objects_tree_read()`
6. `libfsrefs_objects_tree_get_ministore_tree_by_identifier()`
7. `libfsrefs_directory_object_read()`
8. `libfsrefs_directory_entry_read_node_record()`
9. `libfsrefs_file_entry_*()` hierarchy enumeration

This is already the correct high-level path for a newer ReFS file index as well. We should keep this architecture and make the low-level metadata parsing more version-aware.

## Confirmed v3-Aware Areas

These files already contain explicit version branching for v1 and v3 and should be extended instead of replaced:

- `libfsrefs_volume_header.c`
  - accepts major format versions `1` and `3`
  - derives `metadata_block_size` differently for v1 vs v3
  - derives `container_size` for v3
- `libfsrefs_superblock.c`
  - uses v1 vs v3 metadata block header sizes
  - validates the v3 metadata signature `SUPB`
- `libfsrefs_checkpoint.c`
  - switches between v1 and v3 metadata header/trailer sizes
- `libfsrefs_metadata_block_header.c`
  - parses v1 48-byte and v3 80-byte metadata block headers
- `libfsrefs_block_reference.c`
  - parses v1 24-byte and v3 48-byte metadata block references
- `libfsrefs_ministore_node.c`
  - accepts v1 and v3
  - masks the upper 16 bits of v3 record offsets

This means the repository is not purely v1-only at code level. The bigger problem is that the v3 support stops at partial metadata acceptance and does not yet safely propagate into full directory/file enumeration.

## V1-Centric Assumptions Still Blocking v3

### 1. Superblock and checkpoint path is still rigid

File: `libfsrefs_volume.c`

- The superblock is still assumed to live at `30 * metadata_block_size`.
- This may be valid for known versions, but it is still a hard-coded discovery rule.

File: `libfsrefs_file_system.c`

- `libfsrefs_file_system_read_checkpoints()` has `TODO support more than 2 checkpoints`.
- The superblock parser rejects anything other than exactly 2 checkpoints.
- For research purposes we should treat the checkpoint count as data and select the newest valid checkpoint dynamically.

### 2. Container-aware block resolution is incomplete

File: `libfsrefs_file_system.c`

- `libfsrefs_file_system_get_block_offsets()` contains `TODO look up container block range`.
- For v3 volumes with containers, block numbers are not always simple `block_number * metadata_block_size` mappings.
- This is a core blocker because incorrect block resolution corrupts every later tree traversal.

### 3. Multi-block v3 metadata reads are not assembled correctly yet

File: `libfsrefs_ministore_node.c`

- `libfsrefs_ministore_node_read_file_io_handle()` allocates a larger buffer for possible multi-block metadata.
- However, inside the block-read loop each block is read into the start of `ministore_node->internal_data` instead of being appended at an increasing destination offset.
- Result: when a v3 node spans multiple metadata blocks, later reads overwrite earlier ones instead of building a contiguous node buffer.
- This is a direct blocker for v3 object tree and directory object parsing.

### 4. Node record parsing is still structurally generic, not version-aware

File: `libfsrefs_node_record.c`

- `libfsrefs_node_record_read_data()` always parses the same `fsrefs_ministore_tree_node_record_t` layout.
- That is workable only if v3 node record headers are actually identical enough.
- We need to verify this assumption against live v3 samples before relying on directory and object record offsets.

### 5. Directory object traversal assumes a specific record key type

File: `libfsrefs_directory_object.c`

- `libfsrefs_directory_object_read_node()` filters to records where the first key word is `0x0030`.
- This may be valid for known directory entry records, but it is currently a hard-coded semantic assumption.
- We need sample-backed confirmation that newer ReFS still uses the same key prefix for directory member enumeration.

### 6. Directory entry value parsing still assumes fixed layouts

File: `libfsrefs_directory_entry.c`

- `libfsrefs_directory_entry_read_directory_values()` requires `data_size == sizeof( fsrefs_directory_values_t )`.
- This is a strong v1-style assumption and is risky for v3.
- `libfsrefs_directory_entry_read_file_values()` similarly assumes the current file-values node layout and attribute record interpretation remain valid.
- These two functions are likely where the real v3 file-index semantic differences will surface.

### 7. Root object and object record assumptions still need v3 verification

Files:

- `libfsrefs_objects_tree.c`
- `libfsrefs_file_entry.c`

Current assumptions:

- root directory object ID is `0x00000600`
- object tree key is a 16-byte key with the lower 64 bits carrying the object identifier
- object record value can be interpreted as a metadata block reference for the target object tree

The documentation strongly suggests these assumptions remain broadly correct for v3, but we still need live-sample verification because the v3 object record value is described as variable-sized.

## Existing Bugs and Risks Worth Fixing Early

Status update:

- fixed: `libfsrefs_file_system_get_ministore_tree()` error path now returns `-1` instead of `1`
- fixed: `libfsrefs_ministore_node_read_file_io_handle()` now preserves the first metadata block in full and appends the payload of subsequent v3 metadata blocks instead of overwriting the same buffer start
- fixed: `libfsrefs_file_system_read_container_trees()` now keeps the container tree root node cached on the file system object instead of discarding it immediately
- fixed: `libfsrefs_file_system_get_block_offsets()` now performs a first-pass container-aware lookup using container tree records and the container record value field at offset `144` for the physical cluster block number
- still open: we have not yet verified that v3 multi-block nodes need no extra per-block normalization beyond stripping the repeated metadata headers
- still open: container lookup currently assumes the container tree root can answer `get_record_by_key()` directly; if the root is a branch node on larger volumes we will need explicit branch-node traversal support

### 1. Incorrect success return on error path

File: `libfsrefs_file_system.c`
Function: `libfsrefs_file_system_get_ministore_tree()`

- The `on_error` path returns `1` instead of `-1`.
- This can mask failures while testing v3 tree resolution.
- This should be fixed before deeper debugging so failure propagation is trustworthy.

### 2. Multi-block node-read overwrite issue

File: `libfsrefs_ministore_node.c`
Function: `libfsrefs_ministore_node_read_file_io_handle()`

- As noted above, the current read loop overwrites the same buffer start for every referenced block.
- This is likely the single most important low-level correctness issue for v3.

Implementation note:

- The current local fix assembles the node buffer as:
  - first metadata block: copied in full
  - subsequent metadata blocks: only the payload after the metadata block header is appended
- This preserves the existing `libfsrefs_ministore_node_read_data()` contract, which expects to parse a single logical node buffer starting immediately after the first metadata block header.

### 3. Container-aware block resolution

Implementation note:

- The current local fix stores tree `7` (container tree) root node in `file_system->containers_root_node`.
- Tree `8` is still read and validated as the copy of the container tree, then freed.
- `libfsrefs_file_system_get_block_offsets()` now resolves v3 block numbers through the container tree when:
  - format version is 3
  - `container_size` is non-zero
  - the logical block number is greater than or equal to the container size
- The current implementation extracts the physical container start block from container record value offset `144`, matching the current repository documentation.

Remaining risk:

- this is still a minimal implementation and depends on container record lookup working directly from the cached root node
- if real-world v3 samples use a branched container tree, `libfsrefs_ministore_node_get_record_by_key()` will need branch-node support first

## Exact Version-Branching Touch Points

These are the most important functions to convert from "partial version switch" to "fully version-aware parser behavior":

- `libfsrefs_volume_header_read_data()`
- `libfsrefs_superblock_read_data()`
- `libfsrefs_superblock_read_file_io_handle()`
- `libfsrefs_checkpoint_read_data()`
- `libfsrefs_block_reference_read_data()`
- `libfsrefs_metadata_block_header_read_data()`
- `libfsrefs_file_system_get_block_offsets()`
- `libfsrefs_ministore_node_read_file_io_handle()`
- `libfsrefs_node_record_read_data()`
- `libfsrefs_objects_tree_get_ministore_tree_by_identifier()`
- `libfsrefs_directory_object_read_node()`
- `libfsrefs_directory_entry_read_directory_values()`
- `libfsrefs_directory_entry_read_file_values()`

## Recommended Execution Order

1. Fix the obvious correctness bugs first:
   - `libfsrefs_file_system_get_ministore_tree()` error return
   - multi-block node assembly in `libfsrefs_ministore_node_read_file_io_handle()`
2. Make block offset resolution container-aware in `libfsrefs_file_system_get_block_offsets()`
3. Add targeted debug dumping for:
   - object record keys and values
   - directory record keys and values
   - resolved block numbers and offsets
4. Re-validate v3 assumptions for:
   - node record header layout
   - directory entry key type `0x0030`
   - directory value and file value payload sizes
5. Only after the above, build the explicit file-index view.

## Practical Meaning For File-Index Work

At this point the likely path to a ReFS v3 file index is still:

- checkpoint -> ministore tree references -> objects tree -> directory object trees -> directory entry records -> file and directory objects

The main missing pieces are not at the API layer. They are:

- correct v3 block resolution
- correct multi-block metadata assembly
- confirmation of v3 directory and file record payload semantics

Once those are fixed, the existing `file_entry` and `directory_object` abstractions should be reusable for an explicit MFT-like index dump.

## Live-Tested Status On F:

The following has now been validated on the live `\\.\F:` ReFS 3.14 volume:

- volume metadata can be read successfully
- the objects tree can be opened successfully
- the root directory object `0x00000600` can be opened successfully
- directory hierarchy enumeration works for the currently visible test set
- `fsrefsinfo -H \\.\F:` now prints object identifier plus path
- `fsrefsinfo -I \\.\F:` now prints a minimal TSV-style file system index

Current example output:

- `0x00000701    \System Volume Information`
- `0x123d306f5   \System Volume Information\WPSettings.dat`

Additional live test coverage:

- a dedicated test tree was created on `F:\refs-index-test`
- the tree contains nested directories plus files sized: `0`, `7`, `13`, `21`, `37`, `1024`, `4096`, `4097`, `8192`, and `65537` bytes
- `fsrefsinfo -I \\.\F:` was compared against a Windows `Get-ChildItem -Recurse` ground truth listing
- for the current test tree, the exported index now matches Windows for:
  - path presence
  - entry type (`dir` vs `file`)
  - file size

Current index key strategy:

- directory entries use the stable directory object identifier directly
- file entries use a composite key derived from:
  - parent directory object identifier (upper 32-bit)
  - local file identifier recovered from directory-object name records (lower 32-bit)
- this yields unique file index keys across the current live-tested tree and avoids collisions between files in different directories that reuse the same local identifier value

Evidence from live analysis on `F:`:

- the `parent-child relationship tree` currently exposes directory-to-directory relationships, but not the tested file entries
- the tested file entries do not appear as standalone objects in the currently observed `objects tree` root set
- the repeated `file-values` header field at the currently observed offset (`q40`) is not a unique per-file identifier on this volume
- the currently observed `file-values` fields do line up for:
  - `q48` -> file size
  - `q56` -> allocated size / allocation granularity indicator

Current conclusion:

- for the tested ReFS 3.14 volume, a single native per-file object identifier has not yet been confirmed from the explored metadata structures
- the composite key is therefore the best currently validated unique file index key for this volume and test set

Important remaining gap:

- the current file index key is a practical, stable composite identifier for indexing, not yet a fully reverse-engineered canonical on-disk file object identifier for all ReFS v3 layouts
- additional reverse engineering is still needed to prove whether the composite key can be replaced by a single native file object identifier on all newer ReFS volumes

Important current interpretation:

- for directory entries, the stable object identifier comes from the directory-values structure
- for file entries, the stable object identifier comes from the embedded file-values node header data (`identifier_lower` on the currently tested volume)

This means the project has now crossed the threshold from basic ReFS v3 metadata access into a usable file-index style enumeration path.
