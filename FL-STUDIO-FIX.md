# G-AcidBase / FL Studio load-error recovery

## Confirmed diagnosis (4 October 2026)

The installed bundle at `C:\Program Files\Common Files\VST3\G-AcidBase.vst3` is complete and byte-for-byte identical to the tested delivery. Its installed binary successfully scans, instantiates, creates its native editor and renders MIDI audio in the independent JUCE VST3 test host. All tests passed when pointed at this **installed** bundle; see `artifacts/installed-vst3-test.log`.

FL Studio's plugin database still stores the identity of an earlier, different plugin with the same name:

| | Cached FL Studio entry | Current installed plugin |
|---|---|---|
| Vendor | Goa Audio | G-Audio |
| VST3 component ID | `ABCDEF019182FAEB476F616147616231` | `ABCDEF019182FAEB4741634247614231` |

The installed `.nfo` and both saved browser `.fst` entries reference the earlier identity. Recent Plugin Manager logs say `Skipped` and/or `[verified from meta data]` for G-AcidBase rather than actually verifying the new binary. A host requesting the old component ID cannot instantiate the current component.

No plugin-ID change, DSP rewrite or reinstallation is necessary to address this confirmed mismatch. No FL Studio files, registry keys or projects were changed during diagnosis. The plugin binaries are unchanged.

## First recovery: verified rescan

1. Save your current FL Studio project.
2. Remove the failed G-AcidBase instance from the Channel Rack. This failed instance already contains the old ID; rescanning does not rewrite existing instances.
3. Open **Options > Manage plugins**.
4. Enable **Verify plugins**, **Rescan previously verified plugins**, and **Rescan plugins with errors**.
5. Click **Find installed plugins** (or **Start scan**, depending on the FL version). This may take longer than a normal incremental scan because it rechecks existing plugins.
6. Locate G-AcidBase in Plugin Manager. The vendor should now be **G-Audio**, not **Goa Audio**. If it still says Goa Audio, the old entry remains cached.
7. Add a **fresh instance from Plugin Manager or the refreshed Installed > Generators > VST3 entry**, not the old favorited browser shortcut or a previously saved channel preset.
8. If the fresh instance works, replace/recreate the old G-AcidBase favorite using that new instance. Then disable the two rescan options again for faster future routine scans.

Test with low output volume, MIDI note 36 (C2 by this plugin's naming), or SEQ > RUN. A successful scan alone is not proof of successful loading in FL Studio: confirm the editor opens and sound plays.

## If the old identity remains

Save your project and close FL Studio and Plugin Manager first. Back up the following **G-AcidBase-only** browser records outside the plugin database before removing them from their original locations:

```text
%USERPROFILE%\Documents\Image-Line\FL Studio\Presets\Plugin database\Installed\Generators\VST3\G-AcidBase.nfo
%USERPROFILE%\Documents\Image-Line\FL Studio\Presets\Plugin database\Installed\Generators\VST3\G-AcidBase.fst
%USERPROFILE%\Documents\Image-Line\FL Studio\Presets\Plugin database\Generators\G-AcidBase.fst
```

FL Studio can use a custom user-data folder, so use your actual folder if it differs. Reopen FL Studio and perform the verified rescan above. Do not delete the whole plugin database, edit registry keys, reinstall FL Studio, or remove unrelated plugins. If targeted cleanup is desired, ask the coding agent after closing FL Studio so the backup and exact changes can be reviewed and verified safely.

Older projects or channel presets containing the previous plugin ID will still reference that earlier plugin; they must receive a fresh G-AcidBase instance. No claim is made that the new instrument can restore another implementation's saved DSP state.

## Verification boundary

The installed VST3 itself passed the native test host, and the stale FL Studio identity was verified in local cache records and logs. **Successful loading in FL Studio has not yet been confirmed after a fresh scan**; that requires the user to perform the steps above and retry in the running DAW. No change has been made to the user's current FL Studio session.
