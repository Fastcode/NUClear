# TaskQueue wait-freedom assessment (Phase 3 spike)

Branch: `spike/waitfree` (from `houliston/scheduler` @ `d500e324`)

## Definitions (as used here)

| Guarantee | Meaning for a single thread calling `enqueue` / `try_dequeue` |
|-----------|----------------------------------------------------------------|
| **Wait-free** | Completes in a bounded number of its own steps, regardless of other threads. |
| **Lock-free** | The system as a whole makes progress; this thread may retry CAS loops or spin, but no mutual deadlock. |
| **Blocking / unbounded spin** | May yield or spin indefinitely waiting for another thread (not lock-free for that thread in the strict sense, though the queue remains lock-free overall if other threads progress). |

`TaskQueue` is documented as "lock-free MPMC". That is accurate at the **algorithm** level (no mutex; some thread always advances under global progress). It is **not** wait-free end-to-end, and several hot paths deliberately spin for cross-thread handoff.

## Operation-by-operation map (`TaskQueue`)

### `enqueue(T&&)` — producers

| Step | Location | Guarantee | Notes |
|------|----------|-----------|-------|
| Load `tail` | outer loop | Wait-free | Single atomic load. |
| `write.fetch_add(1)` slot claim | fast path | **Wait-free** | One RMW; each producer gets a unique index without CAS. This is the "fetch_add claim" called out in review. |
| Placement-new + `committed.store(release)` | fast path | **Wait-free** | Fixed work after claim; no retry. |
| `link_next_block` | overflow | Lock-free, not wait-free | CAS loop on `block->next`; losers allocate then free a candidate block. Contention bound by concurrent overflow on the same block. |
| `advance_tail` | overflow | Lock-free, not wait-free | CAS loop on `tail`; helping behaviour when another thread linked `next`. |
| Outer `while (true)` on full block | overflow | Lock-free, not wait-free | Unbounded **block** count → unbounded loop iterations if producers continuously fill blocks faster than tail advances. |

**Fast-path enqueue (index < BLOCK_SIZE): wait-free** assuming `T` construction is bounded.

### `try_dequeue(T&)` — consumers

| Step | Location | Guarantee | Notes |
|------|----------|-----------|-------|
| Load `head`, `write`, `read` | loop top | Wait-free | Fixed loads. |
| Empty block: `consumed < published` | stall path | Blocking spin | Waits for other consumers to finish slots; uses `yield` (now `spin_until`). |
| Empty block: `next == nullptr`, producer mid-first-slot | stall path | Blocking spin | Waits for producer commit on slot 0. |
| `read.compare_exchange_weak` | claim slot | Lock-free, not wait-free | MPMC contention on same index; standard CAS retry. |
| Spin on `slot.committed` | after winning read CAS | Blocking spin | Consumer may claim index before producer finishes construct+commit; **inherent to index-then-commit design**. |
| Move + `destroy_slot` | success | Wait-free | |
| `consumed.fetch_add` | success | **Wait-free** | Single RMW. |
| `head` CAS + `retire_block` | block advance | Lock-free, not wait-free | Graveyard push is CAS loop (`retire_block`). |
| `try_reclaim_block` | full block | Lock-free, not wait-free | Head CAS when all slots consumed. |

**Fast-path dequeue (no block transition, no commit wait):** wait-free once `committed` is visible.

### Shared helpers (`detail/block_ops.hpp`)

| Helper | Guarantee |
|--------|-----------|
| `allocate_block` | Not wait-free (`operator new`; system allocator). |
| `link_next_block` | Lock-free CAS; not wait-free under contention. |
| `retire_block` | Lock-free CAS on graveyard head; not wait-free under contention. |

## What is achievable without unbounded preallocation

The queue is **unbounded** in the sense that it allocates a new `Block` (64 slots) whenever the tail block overflows. That implies:

1. **True wait-free MPMC enqueue+dequeue is not achievable** with this block-on-demand design:
   - Block allocation is not wait-free.
   - Overflow paths require CAS on shared list pointers (`next`, `tail`, `head`, graveyard).
   - The `committed` flag exists precisely because `fetch_add` on `write` can run ahead of construction; eliminating the commit spin requires a different slot protocol (e.g. per-slot sequence words, or single-producer lanes).

2. **What we already have (and should keep claiming):**
   - **Wait-free slot claim** on the non-overflow path via `fetch_add` — strong property for producer scalability within a block.
   - **Lock-free** overall: no mutex; failed CAS or spinning threads do not prevent other threads from linking blocks, advancing head/tail, or completing commits.

3. **Bounded preallocation options (not implemented; would change design):**
   - **Fixed-capacity ring:** wait-free ops possible with pre-sized array, but queue becomes bounded and back-pressure policy is needed.
   - **Block pool sized to peak depth:** removes `new` from hot path but requires a priori bound or pool exhaustion handling.
   - **Per-producer SPSC lanes + merge:** wait-free enqueue per producer; MPMC merge at consumer is still hard without spinning or CAS.

4. **Safe improvements without semantic change:**
   - Document progress guarantees accurately (class comment + this spike).
   - **Tighten short spins:** `spin_until` on the `committed` wait (must wait until visible); `pause_and_yield` on outer-loop stall paths (one pause burst + yield per iteration, same control flow as before).
   - Do **not** add hard spin caps that return failure — would change `try_dequeue` contract.

## MPSCQueue (`MPSCQueue.hpp`)

Same producer side as `TaskQueue` (wait-free slot claim on non-overflow). Consumer is single-threaded:

- No read CAS; **dequeue claim is wait-free** once `committed` is visible.
- Still spins on `committed` and on `next == nullptr` while producers link — same handoff pattern, cheaper consumer than MPMC.

For pools with `concurrency == 1`, prefer `MPSCQueue`: strictly simpler consumer with identical producer guarantees.

## Recommendation for the working branch

1. **Keep** block-based unbounded design; lock-free + wait-free slot claim is the right trade-off for the scheduler.
2. **Do not** advertise full wait-freedom; update header comment to match the table above.
3. **Land** `detail::spin_until` for commit/block-wait paths (TaskQueue + MPSCQueue) — micro-latency win, no semantic change.
4. **Defer** any bounded/wait-free queue variant unless a future benchmark shows overflow or commit spins as a measurable bottleneck (unlikely at BLOCK_SIZE=64 for task scheduling).

## Micro-changes in this spike commit

- `docs/spikes/taskqueue-waitfree-assessment.md` (this file)
- `detail/spin_until` and `detail::pause_and_yield` in `block_ops.hpp`
  - `spin_until`: `committed` wait in TaskQueue and MPSCQueue
  - `pause_and_yield`: single-iteration stall paths (unchanged loop structure)
- Expanded progress-guarantee comment on `TaskQueue`
