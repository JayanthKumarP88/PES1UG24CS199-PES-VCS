# PES-VCS (Version Control System) Project
**Name:** Jayanth Kumar P  
**SRN:** PES1UG24CS199  
**Course:** Bachelor of Technology in Computer Science, PES University (2024-2028)

## Phase 1: Object Store
*Implementation of sharded blob storage for file content.*
- **Screenshot 1A (Tests):** ![Phase 1A](./screenshots/WhatsApp%20Image%202026-04-21%20at%2010.47.48.jpeg)
- **Screenshot 1B (Structure):** ![Phase 1B](./screenshots/WhatsApp%20Image%202026-04-21%20at%2017.35.35.jpeg)

## Phase 2: Tree Objects
*Directory serialization and binary representation.*
- **Screenshot 2A (Tests):** ![Phase 2A](./screenshots/WhatsApp%20Image%202026-04-21%20at%2018.13.18.jpeg)
- **Screenshot 2B (Hex Dump):** ![Phase 2B](./screenshots/WhatsApp%20Image%202026-04-21%20at%2018.31.31.jpeg)

## Phase 3: The Index (Staging Area)
*Staging area tracking with metadata and atomic updates.*
- **Screenshot 3A (Status):** ![Phase 3A](./screenshots/WhatsApp%20Image%202026-04-21%20at%2018.17.18.jpeg)
- **Screenshot 3B (Index File):** ![Phase 3B](./screenshots/WhatsApp%20Image%202026-04-21%20at%2018.17.42.jpeg)

## Phase 4: Commits and History
*Commit objects and symbolic reference tracking.*
- **Screenshot 4A (Log):** ![Phase 4A](./screenshots/WhatsApp%20Image%202026-04-21%20at%2018.25.54.jpeg)
- **Screenshot 4B (Growth):** ![Phase 4B](./screenshots/WhatsApp%20Image%202026-04-21%20at%2018.26.25.jpeg)
- **Screenshot 4C (Ref Chain):** ![Phase 4C](./screenshots/WhatsApp%20Image%202026-04-21%20at%2018.31.05.jpeg)

## Final Integration Test
*End-to-end system verification.*
- **Part 1 (Setup & Commits):** ![Final 1](./screenshots/WhatsApp%20Image%202026-04-21%20at%2018.34.07.jpeg)
- **Part 2 (Verification):** ![Final 2](./screenshots/WhatsApp%20Image%202026-04-21%20at%2018.34.24.jpeg)

---

## Phase 5 & 6: Analysis Questions

### Q5.1: Implementing `pes checkout <branch>`
To implement `checkout`, the `HEAD` file must be updated to point to the target branch reference. The working directory must then be synchronized with the snapshot stored in the branch's root tree. This involves deleting files not present in the target tree and overwriting existing files with the content of the target blobs. Safety checks are required to ensure uncommitted changes are not lost.

### Q5.2: Detecting a "Dirty" Working Directory
A "dirty" directory is detected by comparing the current metadata of files (mtime and size) on disk against the metadata stored in the `.pes/index`. If the filesystem's `st_mtime` or `st_size` differs from the index entry, or if a file in the index is missing from the disk, the directory is considered dirty.

### Q5.3: Detached HEAD State
In a "Detached HEAD" state, `HEAD` contains a raw commit hash instead of a branch reference. Commits made here point to the previous hash as their parent, but since no branch reference tracks them, they become orphaned if the user switches branches.

### Q6.1: Garbage Collection Algorithm
I would use a **Mark-and-Sweep** algorithm. Starting from all branch tips and `HEAD`, the algorithm recursively traverses every commit and tree, marking their hashes as "reachable." It then scans `.pes/objects` and deletes any file whose hash was not marked. For 100,000 commits and 50 branches, you would visit at least 100,000 commit objects plus the associated tree hierarchies.

### Q6.2: GC Race Conditions
Running GC concurrently with a commit is dangerous because a new blob might be written seconds before the commit object referencing it is finalized. If GC runs in this window, it would see the blob as unreachable and delete it, resulting in a corrupted commit. Git avoids this by only pruning unreachable objects older than a specific grace period (typically 14 days).
