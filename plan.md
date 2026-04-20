# libfsrefs ReFS v2/v3 Research Plan

## Scope

Goal: extend the `libyal/libfsrefs` research and implementation work beyond ReFS v1 until we can locate and parse the newer ReFS file index structure, i.e. the structure that plays the practical role of an MFT-like file catalog.

Primary outcome:

- identify the canonical metadata path from checkpoint -> objects tree -> directory/file objects -> file index records for ReFS v3.x
- implement enough parsing to enumerate file records reliably from newer ReFS volumes
- validate the parser against live data on `F:`

## Current Baseline

Initial findings were verified from the upstream `main` branch via the GitHub API before clone access was restored. A local working copy is now available at `C:\Users\Administrator\source\libfsrefs`.

- `README` explicitly states read-only support for ReFS `version 1`
- `README` explicitly lists ReFS `version 2` and `version 3` as unsupported
- repository already contains useful abstractions we should preserve instead of redesigning:
  - volume header / superblock / checkpoint parsing
  - metadata block header parsing
  - objects tree parsing
  - ministore tree and node record parsing
  - directory object and directory entry parsing
  - file entry traversal through `libfsrefs_file_entry_*`
- documentation already contains partial reverse-engineering notes for ReFS v3 structures:
  - format v3 metadata block header
  - format v3 metadata block reference
  - format v3 checkpoint trailer
  - format v3 object record value
  - v3-specific object identifiers such as upcase/logfile related objects

Important implementation clue from current code:

- root directory lookup already assumes object identifier `0x00000600` (`REFS_ROOT_DIRECTORY_ID`)
- file hierarchy traversal is built around `objects_tree -> directory_object -> directory_entry -> file_entry`
- this means the shortest path to an MFT-like index is not inventing a new top-level model, but extending the existing object-tree and metadata-block readers to support v3 on-disk layouts first

## Working Hypothesis

For newer ReFS versions, the file index we want is likely not a single flat NTFS-style table exposed as one obvious file. It is more likely represented as:

1. checkpoint-selected metadata roots
2. an objects tree mapping object identifiers to object-local B+-trees
3. directory and file objects represented as ministore trees / records
4. directory entry and file metadata records that together form the practical file catalog

So the target is:

- first parse v3 metadata roots correctly
- then recover stable object references
- then enumerate directory/file objects in a way that gives us a complete file inventory
- finally determine whether there is an even lower-level global object/file table worth exposing as a separate "index" view

## Phased Plan

### Phase 0: Establish local working copy

Status: complete.

Tasks:

- record the exact upstream commit used as research baseline
- verify local build/test entry points
- keep this plan in-repo so implementation notes stay close to code

Exit criteria:

- local editable copy exists
- build/test entry points are runnable locally

### Phase 1: Lock down the existing v1 code path

Tasks:

- read these files end-to-end and map responsibilities:
  - `libfsrefs/libfsrefs_volume*.c`
  - `libfsrefs/libfsrefs_checkpoint.c`
  - `libfsrefs/libfsrefs_objects_tree.c`
  - `libfsrefs/libfsrefs_ministore_node.c`
  - `libfsrefs/libfsrefs_node_record.c`
  - `libfsrefs/libfsrefs_directory_object.c`
  - `libfsrefs/libfsrefs_directory_entry.c`
  - `libfsrefs/libfsrefs_file_entry.c`
  - `fsrefstools/info_handle.c`
- document current assumptions that are v1-only:
  - fixed metadata block header size
  - fixed metadata block reference size
  - object record value layout
  - block-number/reference handling
  - any hard-coded node flags / record sizing assumptions
- identify all places where version branching should occur

Exit criteria:

- a v1/v3 delta checklist exists at function granularity
- we know which functions must become version-aware instead of duplicated

### Phase 2: Build a ReFS v3 format delta matrix

Tasks:

- extract from `documentation/Resilient File System (ReFS).asciidoc` all v3-specific structures into a concise implementation matrix
- normalize the following into a single reference note:
  - volume/version fields
  - metadata block header v1 vs v3
  - metadata block reference v1 vs v3
  - checkpoint trailer/reference differences
  - object record value v1 vs v3
  - object identifier set additions in v3
- mark every field as one of:
  - confirmed
  - partially understood
  - unknown but ignorable for enumeration
  - unknown and blocking

Exit criteria:

- each parser change can be driven by a documented field map instead of ad hoc experimentation

### Phase 3: Make core metadata parsing version-aware

Tasks:

- extend volume/version detection so v3 volumes are accepted for research mode
- update metadata block header parsing to handle v3 80-byte headers
- update metadata block reference parsing to handle v3 48-byte references
- update checkpoint parsing to read v3 reference layout and root pointers
- update any block-number-to-offset resolution logic if v3 indirection differs
- keep v1 behavior unchanged and gated by explicit version checks

Likely code touch points:

- `libfsrefs_volume_header.*`
- `libfsrefs_superblock.*`
- `libfsrefs_checkpoint.*`
- `libfsrefs_metadata_block_header.*`
- `libfsrefs_block_reference.*`
- `libfsrefs_node_record.*`

Exit criteria:

- `fsrefsinfo` can open a v3 volume without failing early in checkpoint or root-tree parsing
- debug output shows stable object-tree roots for v3 samples

### Phase 4: Recover the objects tree for newer ReFS

Tasks:

- verify that tree `0` / objects tree and related root references still resolve correctly on v3
- validate object record key/value parsing for v3 variable-length object values
- expose raw object identifiers and referenced metadata blocks in debug output
- add a tooling mode that dumps object-tree records before higher-level file parsing

Why this phase matters:

- if object resolution is wrong, every later directory/file traversal will look random or incomplete

Exit criteria:

- we can enumerate object identifiers from a v3 volume reproducibly
- the root directory object `0x00000600` resolves on real data

### Phase 5: Reconstruct directory and file indexing semantics

Tasks:

- compare v1 and v3 directory-entry key/value layouts using live samples
- determine whether v3 still stores file and subdirectory membership in directory-object B+-trees or whether an additional indirection layer exists
- identify the stable key used for file inventory:
  - object identifier
  - name hash + name
  - parent-child relation record
  - separate file record object
- add debug dumping for:
  - directory entry keys
  - directory entry values
  - referenced child object identifiers
  - file attribute payloads

Target question for this phase:

- does complete file enumeration come from walking the root directory tree recursively, or is there a separate global object/index tree that is more MFT-like and should be parsed directly?

Exit criteria:

- we can state, with evidence, which on-disk structure is the real file index for ReFS v3
- we can traverse enough of it to enumerate files and directories with object IDs and names

### Phase 6: Implement an explicit file-index view

Tasks:

- add an internal abstraction for the newer ReFS file index, reusing existing structures where possible
- decide output shape for each indexed record:
  - object identifier
  - parent object identifier
  - entry type
  - UTF-8/UTF-16 name
  - timestamps
  - file attributes
  - data or stream references when available
- add a tool mode to dump the index without requiring full recursive pretty-printing

Preferred approach:

- keep `libfsrefs_file_entry` for user-facing traversal
- add a lower-level record enumeration path underneath it if the new index structure is broader than the current directory walk model

Exit criteria:

- a caller can enumerate file-index records from a v3 volume programmatically
- tool output is stable enough to compare against Windows-visible directory listings

### Phase 7: Validation on `F:`

Current observation:

- `F:` is present but currently only shows `System Volume Information/`
- we need to verify whether `F:` is a live ReFS test volume, an empty mount, or a restricted mount point

Validation tasks:

- identify `F:` file system type and version from Windows tools and from `libfsrefs`
- if needed, prepare controlled test data on `F:`:
  - nested directories
  - files with long names
  - alternate timestamps
  - large files
  - renamed and deleted entries if recoverable analysis is in scope
- capture ground truth using Windows enumeration tools
- compare against parser output:
  - root directory object resolution
  - full file count
  - names
  - parent-child paths
  - timestamps
  - attributes

Recommended validation passes:

- smoke pass: open volume, print version, resolve root directory
- traversal pass: recursive listing matches Windows-visible tree
- index pass: explicit file-index dump contains all visible entries
- stress pass: larger directory fan-out and deeper nesting

Exit criteria:

- parser output for `F:` is complete enough to be used as a file index

## Implementation Strategy

Principles:

- keep changes small and version-gated
- prefer making existing parsers version-aware over forking the whole code path
- add debug/dump tooling before guessing semantics
- treat undocumented fields as opaque unless they block traversal or record integrity

Code work order:

1. version detection and metadata headers
2. metadata references and checkpoint roots
3. object record v3 parsing
4. object-tree dump tooling
5. directory/file object traversal for v3
6. explicit file-index abstraction and CLI output
7. tests and live validation on `F:`

## Testing Plan

Automated:

- extend unit tests around:
  - metadata block header parsing
  - metadata block reference parsing
  - object record parsing
  - node-record interpretation for v3 samples
- add fixture-backed tests for known-good v3 metadata blocks as soon as samples are collected

Manual:

- run `fsrefsinfo` in increasingly verbose modes against `F:`
- add temporary debug output during research to correlate object IDs, node keys, and displayed paths
- compare parser results with PowerShell / Explorer listings for the same volume

Artifacts to collect during testing:

- raw sector or image samples for key metadata blocks
- object-tree dumps
- directory-object dumps
- file-index dumps
- expected directory listings from Windows

## Risks and Unknowns

- newer ReFS may not expose one single NTFS-like "MFT" equivalent; the usable file index may be distributed across multiple B+-trees
- some v3 fields are still marked unknown in upstream docs, especially in object record values
- live `F:` access may require elevation or may not currently contain representative ReFS data
- clone/network instability can slow initial setup

Mitigations:

- prioritize dump tooling and empirical sample comparison
- isolate unknown-field parsing from enumeration logic
- keep a research note of every field assumption tied to sample evidence

## Definition of Done

We are done when all of the following are true:

- libfsrefs opens newer ReFS volumes far enough to resolve metadata roots
- we can identify and explain the on-disk structure that acts as the newer ReFS file index
- we can enumerate indexed file records with stable identifiers and paths
- output on `F:` matches Windows-visible file inventory for normal files and directories
- v1 behavior remains intact

## Immediate Next Actions

1. record the exact baseline commit in the repo
2. create a version-delta note from the upstream ReFS asciidoc documentation
3. instrument `objects_tree`, `checkpoint`, and `directory_object` code paths for v3 samples
4. inspect `F:` to confirm whether it is a suitable live ReFS validation target
