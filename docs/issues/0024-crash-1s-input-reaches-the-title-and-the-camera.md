# Crash 1 — input reaches the title; the camera path still does not run

Measured 2026-09-29 on this machine with the disc, all of it by `tools/probe_crash1_input.py`, which
drives the product through the framework's own loopback control channel (`PSXPORT_DEBUG_SERVER=1` plus
`external/psxport/tools/dbgclient.py`). It is a product surface, not a test backdoor: no product
argument changes it.

## 1. The input publisher WORKS, and issue 0012's claim was wrong

    before: pad word 0x80057054 = 0xFFFFFFFF, present frame 78,  frame non-black 0.033
    during: 0xFFFFF7FF  x12 samples                     (bit 11 cleared: Start, active-low)
    after:  pad word 0x80057054 = 0xFFFFFFFF, present frame 1304, frame non-black 0.867
    PASS: the pad word moved to 0xFFFFF7FF, the captured frame content changed, and the
          present counter advanced 78 -> 1304 over the same interval.

`docs/issues/0012` recorded "host input did not reach Crash's BIOS auto-pad word". It did. That
measurement was taken on a product that presented nothing (issue 0022's picture did not exist yet), so
it was a fact about the run rather than about the title.

## 2. The title is reachable and answers input

Holding Start and then tapping walks: the UIS copyright screen, the menu
(`START / LOAD GAME / PASSWORD / OPTIONS`), and then a real level — **"N. SANITY BEACH"**, a full 3D
scene at 512x240 with 1,384 polygons in the final frame, over roughly 4,000 presented frames.

## 3. The camera path still does not run — and the PAUSE was NOT the reason

The first run that reached the level ended showing the game PAUSED, so the obvious suspect was that a
paused title does not update its camera. **It is not that.** The control channel's own `frame` reply
carries `paused=`, and with the taps walked through it reads `paused 0` while the presented frame is
a live, unpaused level:

    N. SANITY BEACH, 512x240, Crash on a path with the HUD, 1,403 polygons in the final frame,
    present frame 3960, paused 0, ~4,000 frames after the hold

and **the owner still publishes a centre 0 times.** So the census holds and its conclusion is wrong
in a way that matters: a level reached by input, running, with a camera on screen, does not call the
widened leaf.

What that leaves, and it is a short list:

* **The census missed an indirect path.** It covered direct `jal`/`j` and `lui`+`addiu`
  materialisations. It did NOT close `jr $ra` / `jalr` through a computed register, and this workspace
  has measured that shape before in this very image: the Spider-Man census reported **0** for every
  library routine because that image reaches its routines through `jalr $ra,$vN` pointers, so a
  `jal`-only view is a property of the scan and not of the title. The same scan run here, restricted
  to direct calls, would report 2 and be true — and would still miss a third route. **This is the
  first thing to check**, because the census's own falsifier section named it and it was never run.
* **The level's camera publishes through a sibling path.** The two callers found are the camera
  distance and centre publishers. The question that was NOT asked is "who writes `CR[24]`", which is
  a different and larger census, and a camera mode can publish its centre somewhere the widened owner
  does not sit.
* **The owner is installed but the key does not fire.** `installCrash1Widescreen` binds a `NativeKey`
  on the leaf's address, and a key built against the wrong image silently never fires — the same
  failure the Crash 1 resume test had to defend against. The install line IS in the log, which proves
  `install()` returned true and proves nothing about whether the key matches the executing image.

## 4. What a control/treatment pair settles, and what it does not

The three candidates above were separated far enough to be worth recording, because the experiment
that separated them is small and the mistake in it is instructive.

**The control.** The control channel's `call A [a0..a3]` command enters a guest function from outside.
Whether that path consults a title's native overrides at all is not documented, so a negative result
through `call` would have said nothing. So the same command was aimed at a site whose override is
**known to fire from the guest** — the projection init at `0x80042B1C`, whose line is in every leg's
log — and at the widened leaf itself:

    call 80042B1C(a0=00001000,...)  -> v0=40000001 v1=40000000
      and the leg then carries a SECOND `guest projection init published` line, with
      `host canvas 512 (native 512)` against the boot's `host canvas 320 (native 320)`.
      Only the owner's override prints that line, so the control FIRED.

    call 80042F8C(a0=0,a1=0,...)    -> v0=00223FFC v1=80081C78
      and the leg carries NO `guest centre` line at all, ever. A second identical call returned
      v0=0022FFFC, so the retail body really ran both times.

**So `call` consults overrides, the mechanism works, the image is the same, and the install loop is
the same — and the `SetGeomOffset` key specifically does not intercept its own leaf.** That is a fact
about the two keys, and it is not the same fact as "the guest never calls the leaf", which is what
issue 0023 concluded. The guest's not calling it may still be true; the owner would not have noticed
either way.

**The two candidates the framework leaves open**, and they are the only two, because
`NativeDispatcher::intercepts` is exactly `isInstalled(key) && !suppressed(key)`
(`runtime/cpu/native_dispatch.cpp`) and the key is `(currentImageIdentity(address), address)`:

1. **The identity resolved at call time differs from the one at install time.** `installOverride`
   resolves the identity once, at install; the dispatcher resolves it again on every entry. A
   generation bump between the two — a module load, a DMA, an image replacement — moves the key's
   image and the leaf stops intercepting, silently, with the install line still in the log.
2. **The key is suppressed and never released.** The dispatcher suppresses a key while a native
   override runs the original, so a suppression pushed and not popped leaves that ONE address dead
   for the rest of the process while every other override keeps working. The boot reaches the GTE
   init site long before the per-frame camera path, so the init key is long-established when the
   camera key would first be exercised — which is the wrong way round for this hypothesis to explain
   the init firing and the leaf not, so (1) is the likelier of the two. That is a prediction, not a
   measurement, and the measurement is cheap: print both identities.

**What would settle it, and it is an assertion rather than a log line:** a test that installs the
three bindings against a real `Core`, then asserts `intercepts()` is true for all three keys both
immediately after install and after a guest field has run. `tests/crash1_widescreen.cpp` already
drives the recovered leaf directly, which is why it never saw this — a direct drive exercises
`publishCentre` and not the dispatch that is supposed to reach it.

Recorded because both are the shape of mistake this workspace keeps making, and because a tool that
prints a verdict next to a contradicting flag is not a tool.

* It read the pad word with `w32`, which is a **WRITE**. The reply echoed the address, so the tool
  read `0xFFFFFFFF` back every sample and would have reported "the host's button never reached the
  word" for a run in which it moved on all twelve samples. The read verb is `rw <addr> [n]`.
* It compared captured frames by **file size**. A fixed-size PPM has a constant length, so a frame
  whose every pixel changed compared equal and the tool reported "the frame did not change" while the
  non-black fraction moved by 0.83. It compares a content digest now.
* Its `frame` parse grepped for `real`, which the reply does not contain, so `advanced` was always
  False — while the PASS branch printed "with the frame counter advancing". The verdict now requires
  every flag it names, and a run that cannot establish one exits 2 as UNUSABLE rather than passing.

Every number above is a log line, and this workspace has been burned four times by a log line that is
absent because nothing feeds the thing that prints it. The absence of `[crash1-wide] guest centre` is
exactly that shape: the owner prints it when it runs, so an absent line is consistent with an owner
that never ran AND with an owner that ran and did not print. Only the owner's own invocation count
distinguishes them, and the owner does not keep one.

## 5. Two defects the tool had, both found by making its own verdict disagree with its own flags

PLACEHOLDER_MOVE

## 6. The instrument this needs, and does not have
