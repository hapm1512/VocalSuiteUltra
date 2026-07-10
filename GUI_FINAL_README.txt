VOCAL CHAIN MASTER - GUI FINAL WORKFLOW PACK

Da xu ly:
- Header SAVE/DEL/A/B/A-B/SET/4X/PWR thanh nut that.
- A/B snapshot chuyen doi va luu trang thai dang chinh.
- SAVE luu User Preset vao Documents/Vocal Chain Master/Presets.
- DEL xoa User Preset.
- Preset panel click duoc 6 factory presets va User Current.
- SET mo menu Undo/Redo/CPU Safe/Analyzer.
- 4X bat/tat oversampling 4x.
- PWR global bypass co fade 20 ms de tranh pop/click.
- SLOPE knob cham va muot hon.
- Tooltip cho header va knob.
- Bo cuc 4 knob: 3 tren + 1 giua.
- Bo cuc 5 knob: 3 tren + 2 hai ben.
- Doi CORRELATION thanh STEREO.

Build:
cmake -S . -B Build -G "Visual Studio 18 2026" -A x64
cmake --build Build --config Release

Kiem tra:
1. Click 6 factory preset duoi panel.
2. SAVE, sau do click USER - CURRENT.
3. DEL xoa user preset.
4. Chinh thong so o A, click B, chinh khac, click A/B.
5. Click 4X va kiem tra den active.
6. Click PWR, am thanh bypass khong bi pop.
7. Van SLOPE 12/24/36/48 cham hon.
