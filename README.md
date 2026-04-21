# PES-VCS (Version Control System) Project
**Name:** Jayanth Kumar P  
**SRN:** PES1UG24CS199  
**Course:** Bachelor of Technology in Computer Science, PES University (2024-2028)

## Phase 1: Object Store
*Implementation of blob storage with sharded directory structure.*
- **Screenshot 1A:** ![Phase 1A](./screenshots/163090df%20f02a%204314%20857e%202fca99ea9796.jpg)
- **Screenshot 1B:** ![Phase 1B](./screenshots/1c098c35-a7aa-4439-af1a-e90612cbd03b.jpg)

## Phase 2: Tree Objects
*Implementation of directory serialization and recursive tree construction.*
- **Screenshot 2A:** ![Phase 2A](./screenshots/2741ccdd-e938-43bf-97de-9dc0cc2f3565.jpg)
- **Screenshot 2B:** ![Phase 2B](./screenshots/8ab175ac-6fe3-435e-a775-5a831bfe3201.jpg)

## Phase 3: The Index (Staging Area)
*Implementation of the staging area with atomic writes and change detection.*
- **Screenshot 3A:** ![Phase 3A](./screenshots/8cbb7f41-9d30-47a3-90cd-8b8be9746a3e.jpg)
- **Screenshot 3B:** ![Phase 3B](./screenshots/b52f1c5c-81ca-4823-3000-89ec62352903.jpg)

## Phase 4: Commits and History
*Implementation of commit creation and linked history pointers.*
- **Screenshot 4A:** ![Phase 4A](./screenshots/c8728a0b-4c28-4e7e-8991-9059f7df0167.jpg)
- **Screenshot 4B:** ![Phase 4B](./screenshots/d17e19d4-0050-447b-883e-51bde8dcdf9d.jpg)
- **Screenshot 4C:** ![Phase 4C](./screenshots/d83a7346-c6f3-4421-80e4-847700ba8407.jpg)

## Final Integration Test
*Full system test showing successful end-to-end VCS operations.*
- **Part 1:** ![Final 1](./screenshots/e0a6b240-ca18-46f2-8015-a41f7c871f9f.jpg)
- **Part 2:** ![Final 2](./screenshots/f7567e99-5565-4b63-9b6b-e395aca416a5.jpg)

---

## Phase 5 & 6: Analysis Questions

### Q5.1: Implementing `pes checkout <branch>`
To implement `checkout`, the `HEAD` file must be updated to point to the target branch reference. The working directory must then be synchronized with the snapshot stored in the branch's root tree. This involves deleting files not present in the target tree and overwriting existing files with the content of the target blobs. The operation is complex because it must handle safety checks to ensure uncommitted changes in the current working directory are not lost.

### Q5.2: Detecting a "Dirty" Working Directory
A "dirty" directory is detected by comparing the current metadata of files (mtime and size) against the metadata stored in the `.pes/index`. If the filesystem's `st_mtime` or `st_size` differs from the index entry, or if a file in the index is missing from the disk, the directory is dirty.

### Q5.3: Detached HEAD State
In a "Detached HEAD" state, `HEAD` contains a raw commit hash instead of a branch reference. Commits made here point to the previous hash as their parent, but because no branch reference tracks them, they can become orphaned if the user switches away. They can be recovered by manually creating a new branch at that specific hash.

### Q6.1: Garbage Collection Algorithm
I would use a **Mark-and-Sweep** algorithm. Starting from all branch tips and `HEAD`, the algorithm recursively traverses every commit and tree, marking their hashes as "reachable." It then scans `.pes/objects` and deletes any file whose hash was not marked. For 100,000 commits and 50 branches, you would visit at least 100,000 commit objects plus the associated tree hierarchies.

### Q6.2: GC Race Conditions
It is dangerous to run GC concurrently with a commit because a new blob might be written seconds before the commit object that references it is finalized. If GC runs in this window, it would see the blob as unreachable and delete it, resulting in a corrupted commit. Git avoids this by only pruning unreachable objects older than a specific grace period, typically 14 days.
