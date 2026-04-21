# PES-VCS (Version Control System) Project
**Name:** Jayanth Kumar P  
**SRN:** PES1UG24CS199  
**Course:** Bachelor of Technology in Computer Science, PES University (2024-2028)

## Project Overview
This project is a custom implementation of a Git-inspired Version Control System (VCS) called `pes`. It features a content-addressable object store, tree-based directory representation, and commit history traversal.

---

## Phase 1: Object Store
*Implementation of blob storage with a sharded directory structure.*
- **Screenshot 1A:** ![Phase 1A](./screenshots/163090df-f02a-4314-857e-2fca99ea9878.jpeg) - `./test_objects` output showing passing tests.
- **Screenshot 1B:** ![Phase 1B](./screenshots/1c098c35-a7aa-4439-af1a-e90612cb30ce.jpeg) - `find .pes/objects -type f` showing the sharded directory structure.

## Phase 2: Tree Objects
*Implementation of directory serialization and recursive tree construction.*
- **Screenshot 2A:** ![Phase 2A](./screenshots/2741ccdd-e938-43bf-97de-9de0cc2f0269.jpeg) - `./test_tree` output showing successful serialization.
- **Screenshot 2B:** ![Phase 2B](./screenshots/8ab175ac-6fe3-435e-a775-5a831bfe3201.jpeg) - `xxd` hex dump of a raw object showing binary formatting.

## Phase 3: The Index (Staging Area)
*Implementation of the staging area with atomic writes and change detection.*
- **Screenshot 3A:** ![Phase 3A](./screenshots/8cbb7f41-9d30-47a3-90cd-8b8be9739556.jpeg) - Output of `pes status` showing staged test files.
- **Screenshot 3B:** ![Phase 3B](./screenshots/b52f1c5c-81ca-4823-3000-89ec623528f4.jpeg) - `cat .pes/index` showing the text-format index metadata.

## Phase 4: Commits and History
*Implementation of commit creation, author metadata, and history traversal.*
- **Screenshot 4A:** ![Phase 4A](./screenshots/c8728a0b-4c28-4e7e-8991-9059f7dfc026.jpeg) - `./pes log` output showing the commit chain.
- **Screenshot 4B:** ![Phase 4B](./screenshots/d17e19d4-0050-447b-883e-51bde8d122b0.jpeg) - `find .pes -type f` showing object store growth.
- **Screenshot 4C:** ![Phase 4C](./screenshots/d83a7346-c6f3-4421-80e4-847700ba8407.jpeg) - Verification of the symbolic reference chain.

## Final Integration Test
*Comprehensive system test proving end-to-end functionality.*
- **Part 1:** ![Final Part 1](./screenshots/e0a6b240-ca18-46f2-8015-a41f7c87c06c.jpeg) - Initial repo setup, staging, and early commits.
- **Part 2:** ![Final Part 2](./screenshots/f7567e99-5565-4b63-9b6b-e395aca41f23.jpeg) - Full history log, reference chain, and object store verification.

---

## Phase 5 & 6: Analysis Questions

### Q5.1: Implementing `pes checkout <branch>`
**Answer:** To implement `checkout`, the `HEAD` file must be updated to point to the target branch reference. The working directory must then be synchronized with the snapshot stored in the branch's root tree. This involves deleting files not present in the target tree and overwriting existing files with the content of the target blobs. The operation is complex because it must handle safety checks to ensure uncommitted changes in the current working directory are not lost.

### Q5.2: Detecting a "Dirty" Working Directory
**Answer:** A "dirty" directory is detected by comparing the current metadata of files (mtime and size) against the metadata stored in the `.pes/index`. If the filesystem's `st_mtime` or `st_size` differs from the index entry, or if a file in the index is missing from the disk, the directory is dirty.

### Q5.3: Detached HEAD State
**Answer:** In a "Detached HEAD" state, `HEAD` contains a raw commit hash instead of a branch reference. Commits made here point to the previous hash as their parent, but because no branch reference tracks them, they can become orphaned if the user switches away. They can be recovered by manually creating a new branch at that specific hash.

### Q6.1: Garbage Collection Algorithm
**Answer:** I would use a **Mark-and-Sweep** algorithm. Starting from all branch tips and `HEAD`, the algorithm recursively traverses every commit and tree, marking their hashes as "reachable." It then scans `.pes/objects` and deletes any file whose hash was not marked. For 100,000 commits and 50 branches, you would visit at least 100,000 commit objects plus the associated tree hierarchies.

### Q6.2: GC Race Conditions
**Answer:** It is dangerous to run GC concurrently with a commit because a new blob might be written seconds before the commit object that references it is finalized. If GC runs in this window, it would see the blob as unreachable and delete it, resulting in a corrupted commit. Git avoids this by only pruning unreachable objects older than a specific grace period, typically 14 days.
