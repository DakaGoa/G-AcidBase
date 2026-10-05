# G-AcidBase

A native Windows x64 VST3 and standalone monophonic acid bass instrument for Goa trance. Original 303-inspired synthesis; no proprietary assets or DSP from the pictured instrument are used. The visual design is dark graphite, acid green, cyan and violet.

## Delivery

After building and packaging:

- `dist/G-AcidBase.vst3/` — **copy this entire folder**, not just its internal binary.
- `dist/G-AcidBase.exe` — standalone instrument, with JUCE's audio/MIDI settings menu.
- `dist/Presets/` — exactly 50 portable `.gacid` factory preset files.
- `dist/G-AcidBase-Windows-x64.zip` — plugin, standalone, presets, logo and documentation.
- `dist/Logo/` — logo assets: `G-AcidBase-Logo.svg` plus 64/128/256/512/1024 px PNG rasters.
- `dist/index.html` — browsable build preview with the logo, editor screenshot and 50-preset audio.
- `artifacts/G-AcidBase-50-Preset-Demo.wav` — 75-second, 24-bit stereo preset audition at 48 kHz (1.5 seconds per preset).
- `artifacts/G-AcidBase-Interface.png` — screenshot captured from the actual native editor.
- `artifacts/verification.txt` and `artifacts/preset-audio-measurements.csv` — automated verification and measured audio levels.
- `assets/G-AcidBase-Logo.svg` and `assets/G-AcidBase-Logo-{64,128,256,512,1024}.png` — the logo, regenerated with `python scripts/make_logo.py`.

### Logo

One geometry spec drives both the logo inside the editor (`Source/Logo.h`, drawn as vectors at any size) and the shipped files (`scripts/make_logo.py` writes the SVG and the PNG rasters). The mark is a graphite rounded badge with an acid-green saw wave, a cyan resonant filter slope with its corner dot, and a violet control knob, using the same five colours as the interface. The 512 px PNG is also compiled into the VST3 and standalone as the Windows application/plugin icon.

After editing either the generator or the header constants, regenerate the assets and run the tests: the feature suite compares the two renders pixel for pixel (currently 99.97% of 262,144 pixels within a small channel tolerance, differing only on anti-aliased edges) and fails if the geometry or palette drifts apart.

The header mark is animated on top of that artwork. One cycle lasts exactly one host tempo beat, so the badge swells and brightens on the beat, an echo of the saw travels across it, and the **Resonance** control fattens the cyan filter slope and opens its end dot like a filter coming alive. Everything is vector, clipped to the badge and drawn from the same state the editor already repaints at 25 Hz, so it costs a few strokes on a 48 px square and no extra timer. `artifacts/G-AcidBase-Logo-Animation.png` shows one beat across the top and the same beat with the filter fully open underneath. The tests pin the animation when saving the header snapshot, check that motion never leaves the badge, and check that the resonance response touches only the filter slope.

### Updates

**TOOLS → CHECK FOR UPDATES** reads one anonymous `GET` of `https://y4m4.github.io/GoaSynth/gacidbase/version.json` — the only request the plugin ever makes — on a background thread, and always answers in a dialog over the plugin window: the feed could not be reached, a newer release is available, or you are up to date. The update-available card carries the release date, what changed and the download size, and its two rows link straight to the new package on GitHub and to that release's notes. Every outcome is decided by pure functions in `Source/UpdateCheck.h`, so the wording, the links and the version comparison are tested without a network.

The feed is published beside the product page in the site repository. `scripts/package.py` refuses to package a build whose version the feed does not name, so the check inside a shipped plugin can never offer an update to itself or miss one that exists.

### About

The header's version line (`G-AUDIO / <version>`) is a button: click it for the About card, which shows the running version and links to the [product page](https://y4m4.github.io/GoaSynth/gacidbase/), the [source repository](https://github.com/Y4m4/G-AcidBase) and the [current release](https://github.com/Y4m4/G-AcidBase/releases/latest), along with how JUCE is licensed and the VST trademark note. The version on the card is read from the same constant the update check compares against, so it cannot describe a build other than the one running. Escape, the close button or clicking outside the card dismisses it.

## Install / first sound

1. Close your DAW.
2. Copy `dist/G-AcidBase.vst3` to `%LOCALAPPDATA%\Programs\Common\VST3\` if your DAW supports this user location, or add `dist` to its custom VST3 scan paths. The machine-wide standard is `C:\Program Files\Common Files\VST3\` (Windows may ask for administrator permission when **you** copy there).
3. Rescan plugins, then load **G-AcidBase as an instrument** on a MIDI/instrument track, not as an audio effect.
4. Start with low monitor/headphone volume. The default is MIDI mode. Send C2 (MIDI note 36), use the on-screen keyboard, or toggle **AUDITION**.
5. For the factory acid pattern, choose **SEQ**, then **RUN**. Select any of the 50 presets. RUN is a manual latch; switch it off to stop. Choosing a preset returns to MIDI mode and does not change the RUN latch.
6. DAW tempo takes precedence over the internal tempo when available. When the host is playing, sequence phase follows host PPQ; when stopped or standalone, the internal clock runs. This is explicitly triggered by RUN or held MIDI, not automatically by the host Play button.

The standalone app has an **Options** button for selecting audio outputs and MIDI input devices. It does not need a DAW. If Windows warns about an unsigned executable, this local build is not code-signed; do not disable system-wide security settings.

### FL Studio reports “plugin failed to load”

If G-AcidBase was previously scanned from another build, a normal incremental scan can retain an obsolete VST3 ID. In **Options > Manage plugins**, enable **Verify plugins**, **Rescan previously verified plugins** and **Rescan plugins with errors**, then scan again. Add a **fresh instance** from the refreshed installed-plugin entry, not an old favorite or saved failed instance. The current vendor is **G-Audio**. See `FL-STUDIO-FIX.md` for the confirmed stale-ID diagnosis and safe recovery. Do not delete unrelated plugin caches or change the current plugin ID to match a different implementation.

## Sound engine

- 4x oversampling with JUCE polyphase IIR half-band filters and reported integer latency.
- PolyBLEP saw and variable-pulse-width square oscillators, with subtle noise/click trim.
- Four-pole topology-preserving resonant ladder with nonlinear saturation and fundamental compensation.
- Filter envelope, accent filter/amp envelopes, gate attack/release and pitch glide.
- Monophonic last-note priority. Overlapping MIDI notes slide; MIDI velocity >= 102 accents.
- Pitch bend +/-2 semitones; CC1 adds vibrato; all-notes-off releases held notes.
- Four distortion models (diode, asymmetric, tube-like, sine fold), pre/post filter routing, dynamics, drive and tone color.
- Stereo ping-pong delay with beat sync and filtered feedback; diffuse stereo reverb with predelay; three-band peaking EQ; stereo chorus.
- Smooth gain changes, DC blocking and an optional stereo-linked, zero-lookahead peak limiter. The limiter is intentionally bypassed with BYPASS FX. It is **not** a true-peak mastering limiter.
- Audio thread buffers allocated during prepare; processing does not create buffers or delay-line allocations.

This is an original musical interpretation, **not** a circuit-identical TB-303 model. “Best sound quality” is subjective: measurements check safety/correctness, not artistic preference or equivalence to hardware. Circuit trim labels correspond to the reference image, but are musical mappings rather than electrically calibrated component simulation.

## Controls from the reference

| Reference | G-AcidBase location / behavior |
|---|---|
| Scene + synthesis presets / save | One unified 50-preset browser stores synthesis, FX and the full scene/pattern. SAVE/LOAD exports/imports `.gacid`. No separate undocumented scene format. |
| Wave, tuning, cutoff, resonance, envmod, decay, accent | SYNTHESIS main row. |
| Slide time, sweep, env attack, acc decay, acc vol, vib speed/depth | SYNTHESIS main row (slide also available in PLAY MODE). |
| TM3, C21/22, ACCENV, ACCAMP, C13, TM5, SQR PW, BA662 clicks/noise | CIRCUIT TRIMS reveals nine host-automatable mappings: tuning trim, envelope shape, accent amounts, saturation bias, key tracking, pulse width, click/noise levels. |
| EXT / SEQ / ARP | MIDI / SEQ / ARP play mode. MIDI is the external note-input mode; no external audio-to-CV input. |
| Distortion active / model / dynamics / order / drive / color | DISTORTION panel. Pre/post means before/after the resonant filter. |
| Reverb / EQ / delay / chorus / limiter / bypass FX | FX tabs each have ENABLE; limiter and bypass are in OUTPUT. Switching FX pages does not change their enable state. |
| Predelay / early-late / feedback / FX | Reverb page: predelay, diffusion/damping balance, decay (room size), mix. This is not literal early-reflection impulse modeling. |
| Volume / output meters | OUTPUT gain (-60 to +12 dB), stereo peak meters. |
| MIDI MOD MATRIX | Configure velocity, channel aftertouch, and CC1 mod wheel to modulate accent, filter cutoff (semitones), reverb mix, and FX/output gain. Depths are host-automatable and saved with presets/state. |
| MIDI Learn / macros | Right-click a continuous control for a dismissible menu: MIDI Learn, reset, remove/change CC mapping, or assign a macro depth. Select MIDI Learn, then move a CC. Mappings are channel-specific and saved in state/presets; choices/toggles are excluded. CLOSE, Escape or Cancel MIDI Learn exits waiting learn without deleting an existing map. |
| More / settings | FX tabs reveal detailed controls; circuit tab reveals trims. Standalone Options selects devices. |

### Sequencer / arpeggiator

16 step columns each have a semitone offset (-12 to +24), GATE, ACC and SLIDE. SLIDE ties the active step into the following active step. Rests break ties. Root sets the base note unless incoming MIDI transposes it. Global gate length, swing and 1/8–1/32 rate are available. MUTATE uses the scale/density/variation selected in TOOLS > PATTERNS; RESET sets repeated accented roots. Both preserve locked steps. In ARP mode held MIDI notes replace the sequence pitches, with Up/Down/Up-down direction and 1–3 octaves; step gate/accent/slide still apply.

Double-click knobs to reset them. Shift-drag offers finer adjustment. Every sound, clock and FX control is host-automatable; pattern cells persist in DAW state and `.gacid` files but are not individual host automation parameters. MIDI velocity, channel/poly aftertouch, and CC1 mod wheel drive the modulation matrix. MIDI Learn is selected from the right-click menu on continuous controls; mappings persist in DAW state and `.gacid` files. The editor is resizable from 960×705 to 5120×3760 logical pixels and honors the host scale factor (up to 4×), suitable for high-DPI/4K setups.

### New sound-design and composition tools

**TOOLS** opens four pages above the main controls. **CLOSE**, TOOLS again, or **Escape** returns to synthesis; the MIDI matrix also has its own CLOSE and CANCEL LEARN buttons. Context menus close with their Close menu entry, Escape, or a click outside. No separate Performance Mode is included.

- **LFOs:** two bipolar sources; sine, triangle, saw and square; free rate 0.05–20 Hz or tempo divisions from 1/16 to four bars. Each routes to filter (±48 semitones), accent, reverb and FX/gain. Retrigger resets phase on MIDI/sequence notes. Synced, non-retriggered LFOs follow host PPQ during playback; otherwise phases run on the internal clock.
- **Macros / morph:** four host-automatable macro amounts. Right-click a sound knob to assign one or more macro depths (negative or positive normalized parameter-range offsets). Multiple destinations per macro are supported; cyclic macro targets and discrete switches are excluded. At amount zero, macros leave the base value unchanged. Capture MORPH A, change the sound, capture MORPH B, then move Sound morph. Interpolation is in normalized sound-parameter space, respecting logarithmic knob ranges; discrete sound switches change halfway. Morph affects synthesis/FX/output, not transport, MIDI maps or patterns. While both snapshots exist they override base sound values; CLEAR MORPH returns to the underlying knobs. Macro amounts are additive after morph. Knobs display underlying host parameter values, not modulation results.
- **Pattern banks / chaining:** eight editable 16-step banks. EDIT BANK selects a bank; when chaining is off it is also the playback bank. Enable chain, select length 1–8, and choose each entry's bank and repeat count 1–16. Playback repeats the complete chain. Pattern banks, editing selection and chain entries persist in presets and host state.
- **Constrained generator:** Phrygian, minor, major or Dorian-pentatonic pitches; note density controls gates; variation controls how much is replaced. Seed is host-automatable and advances on generation. Right-click any step to lock it against generation/reset. Unlocked step probability/ratchets/microtiming are preserved during pitch/rhythm generation.
- **Advanced steps:** right-click a step for 0/25/50/75/100% probability, 1–4 ratchets, ±25% microtiming in 5% increments, and lock. Timing is an offset in step units; negative timing anticipates the grid. Probability is seeded and deterministic per absolute step, so host seeks and MIDI export agree. Ratchets retrigger envelopes; SLIDE ties the final pulse into the next playing step. First exported anticipations are clipped to tick zero.
- **MIDI export:** EXPORT MIDI saves a format-0 `.mid` with 960 ticks per quarter, internal tempo metadata, current root/channel, swing/timing, ratchets, velocity accents and overlapping slide notes. Exports one bank when chaining is off or one complete chain when enabled. DRAG MIDI TO DAW creates a temporary file and initiates native file drag-out; receiving DAW support varies. Export writes sequencer notes, not audio, pitch-glide curves, modulation/FX automation, or live ARP-held pitches. Use file import if your DAW cannot accept a drop. Drag files remain in the OS temporary directory so a receiver can finish reading them.
- **Controller modes:** Jump (absolute), Pickup (soft takeover), Relative signed (1 increases, 127 decreases, 0/64 neutral), and Relative offset (65 increases, 63 decreases, 64 neutral). Set the default in CONTROLLERS before learning; existing maps have a right-click Controller mode submenu. Pickup rearms after the target moves away from the controller's last delivered value. Relative encoders add 1/127 of normalized range per tick. CC changes are coalesced and delivered to host parameters by the message-thread timer, not APVTS writes in the audio callback.
- **Undo/redo / A-B:** UNDO/REDO restore up to 64 full edit checkpoints, including sound, patterns, maps and snapshots. Native knob gestures, buttons, menus and generation create checkpoints; Ctrl+Z/Ctrl+Y work when the editor has focus. STORE A/B initializes two identical full-state comparisons; A/B switches and remembers independent edits on each side. Edit history/comparison slots are session tools, not saved preset history, and continuous external host automation is not individually recorded.
- **Automation safety:** continuous base sound parameters and macro/morph offsets are smoothed over approximately 15 ms, with additional voice/gain smoothing. Discrete switches intentionally remain immediate. Legacy presets receive default values for new controls and full-probability, one-ratchet, zero-offset steps. AUDITION/on-screen keyboard and legacy CC1 vibrato are regression-tested.

### Factory banks (50 total)

1. **01–10 / Goa foundations:** Anjuna Sunrise through Full Moon Ritual.
2. **11–20 / Psychedelic movement:** Bamboo Pulse through Green Vortex.
3. **21–30 / Driven acid:** Sandstorm Drive through Resonant Beast.
4. **31–40 / Space and echoes:** Midnight Dub through Fractal Stream.
5. **41–50 / Low-end and classics:** Subterranean through Final Ascension.

Each program has a unique voiced state and 16-step pattern. Factory presets are built into the plugin; importing the files is optional. Program changes select factory sounds; custom SAVE files preserve the currently edited state without overwriting the embedded factory bank.

## Build / test (Windows x64)

Requirements: Visual Studio 2022 C++ Build Tools + Windows SDK, Git and CMake >= 3.22. JUCE 8.0.9 is pinned to commit `f72bad64d29715216226685810c5196bd0d79d77` and fetched on first configure.

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel 4
ctest --test-dir build -C Release --output-on-failure
python scripts/make_logo.py   # regenerate logo assets (only needed after editing the logo)
python scripts/package.py
```

Tests load the **actual VST3 bundle**, instantiate its editor and render MIDI through the wrapper. The suite also renders all 50 presets; validates finite samples, audible output, limiter bounds and DC; checks host state/pattern roundtrip; tests MIDI notes/legato/bend/panic at 44.1/48/96 kHz and 64/511/2048-sample blocks; verifies exact MIDI onset and arp start/stop. The expanded feature suite checks CC pickup/relative modes and cancellation, extended state and legacy preset migration, generator locks/scales, chains, parsed MIDI export, macro/morph/LFO audio, probability/ratchet rendering and block partitioning, undo/redo/A-B, native context-menu dismissal, panel Close/Escape and DPI transforms. Its separate report is `artifacts/feature-verification.txt`. Actual DAW drag-and-drop/automation behavior, physical high-DPI hardware and listening approval still require testing in your DAW. No unmeasured claims of hardware accuracy, anti-aliasing superiority, or “best possible” sound are made.

## Licensing / redistribution

JUCE 8 modules are dual-licensed **AGPLv3 or a commercial JUCE license**. This checkout is a local development result and does not assign a new license to your own source. Before distributing binaries, choose and comply with an applicable JUCE license (including publishing corresponding source if using the AGPL route), or obtain an appropriate commercial license. See `build/_deps/juce-src/LICENSE.md`. Bundled VST3 SDK and other dependency notices also apply; packaging includes JUCE's third-party notices. No paid service or account was provisioned. This build is unsigned and not installed globally by the agent.
