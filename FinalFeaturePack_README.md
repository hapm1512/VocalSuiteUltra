# VocalSuiteUltraPro - Final Feature Pack

Changes added in this package:

- Factory preset infrastructure
- Six factory preset definitions
- A/B state snapshot hooks
- Undo/Redo hook through APVTS UndoManager
- CPU Safe Mode parameter
- Analyzer enable parameter
- Oversampling selection hook: Off / 2x / 4x
- Pitch Correction hook parameters
- LicenseManager skeleton
- CMake integration for Preset and License sources

Notes:

- Pitch Correction is a hook only, not a full pitch correction algorithm.
- Oversampling is exposed as a parameter hook, ready for DSP integration.
- Existing DSP chain is preserved.
