# DSP Refinement Pass 1

- Reordered the main chain for safer gain flow:
  Noise -> Gate -> HPF -> Preamp -> Vocal EQ -> De-Esser -> FET1176 -> Saturation -> Hi-End -> Limiter.
- Reduced preamp headroom recovery so high Drive no longer feeds excessive level downstream.
- Replaced fixed ratio-based compressor makeup with actual gain-reduction-based makeup.
- Added a soft safety ceiling after FET compression.
- Added a soft safety ceiling after saturation.
- Added automatic Hi-End loudness trim.

Build on Windows with JUCE 8 and Visual Studio 18 2026.
