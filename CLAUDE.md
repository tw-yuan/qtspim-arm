# CLAUDE.md

本檔給在此 repo 工作的 AI agent 閱讀。完整任務與各階段驗收條件見 `docs/PORTING_PLAN.md`，兩者衝突時以本檔的「硬性規則」為準。

## 專案

- **qtspim-arm**：SPIM / QtSpim（James R. Larus 的 MIPS32 模擬器，BSD 3-Clause）的**非官方 fork**。
- 目標：macOS 原生 Universal Binary（arm64 + x86_64）、Qt 6、Developer ID 簽章與公證、內建程式編輯器、Apple 風格介面。
- 上游：SourceForge SVN `https://svn.code.sf.net/p/spimsimulator/code`，本 repo 由 `git svn` 匯入至 r764（9.1.24，2023-08-04）。
- Repo 目前為**私人**，擁有者 tw-yuan，已決定盡快公開（2026-10-12）。
- 擁有者已做的決定（Bundle ID、App 名稱、版本號、最低 macOS 等）記在 `docs/PORTING_PLAN.md` 的「擁有者決定紀錄」，照著做，不要再自行更改。

## 目錄

| 路徑 | 內容 | 修改原則 |
|---|---|---|
| `CPU/` | 模擬核心：指令、記憶體、syscall、`parser.y`、`scanner.l`、`exceptions.s` | **盡量不動**，見硬性規則 |
| `QtSpim/` | Qt 圖形介面：新介面（`mainwindow.cpp`、`simulator.cpp` 等）與經典介面（原版 `spimview.cpp` 等，可互相切換）；`translations/` 為繁中翻譯 | 主要工作區 |
| `spim/` | 命令列版，`Makefile` 含 `test` / `test_bare` | 用來產生行為基準 |
| `Tests/` | 上游 torture tests（`tt.*.s`、`tt.in`） | 不要修改 |
| `regression/` | 回歸測試：`manifest.txt`、`cases/`、`golden/`、說明 | golden 只能用未修改的原版 r764 錄製（例外見 `regression/README.md`） |
| `Documentation/`、`QtSpim/help/` | 文件與說明檔 | 可更新 |
| `Setup/`、`PCSpim/`、`xspim/` | 舊打包設定、Windows 版、X11 版 | **不在範圍內，不要修改或刪除** |

## 建置與測試

macOS 內建的 bison 是 2.3，太舊。一律使用 Homebrew 版：

```bash
brew install qt cmake bison flex
export PATH="$(brew --prefix bison)/bin:$(brew --prefix flex)/bin:$PATH"
```

命令列版（上游 Makefile，目前可用）：

```bash
cd spim && make && make test
```

`make test` 會印出每個 torture test 的最後兩行，結果應為全部通過。

CMake 建置（細節見 `docs/BUILDING.md`）：

```bash
cmake -B build -G Ninja -DCMAKE_PREFIX_PATH=<Universal 版 Qt 6.11>   # macOS 預設 arm64;x86_64，最低 macOS 13
cmake --build build
scripts/run-tests.sh build/spim     # 與 regression/golden/ 比對，有差異即失敗（或 ctest --test-dir build）
lipo -info build/spim "build/QtSpim ARM.app/Contents/MacOS/QtSpim ARM"
cmake --build build --target update_translations   # 改了介面字串後更新 QtSpim/translations/*.ts
```

- Homebrew 的 Qt 只有本機架構，做不出 Universal 的 QtSpim。要用 Qt 官方安裝程式或 aqtinstall 的 Qt，否則就加 `-DCMAKE_OSX_ARCHITECTURES=arm64`。
- 測試一律用 `env -i` 執行 spim（spim 會把環境變數複製到模擬堆疊上），而且不能用 `alarm()` 包裝（spim 用 `ITIMER_REAL` 驅動 CP0 計時器）。見 `regression/README.md`。
- `fp_host_dependent`、`fp_host_edge` 記錄了會隨主機 CPU 而不同的浮點結果（NaN 位元、超出範圍的浮點轉整數）。`CPU/run.cpp` 已讓所有主機都產生 x86 的結果，所以在 arm64 上也必須通過；**不要改 golden**。

參考實作：`brew install spim` 的 9.1.24 就是用 r764 編譯的，可拿來對照輸出（Apple Silicon 上是 arm64 版，`fp_host_dependent` 會不同）。

## 硬性規則

1. **行為必須與原版一致。** 這是教學工具，學生作業由助教用原版 SPIM 評分。指令語意、syscall、例外處理、延遲槽、記憶體配置、輸出格式的任何改變都算 bug。
2. **`CPU/` 只允許為了編譯通過所需的最小修改**，每一處都要在提交訊息中說明原因。不要重構、重新格式化或「順手改善」核心程式碼。
3. **不要修改 golden 輸出讓測試通過。** 輸出和原版不一致時，找出原因並回報。
4. **任何修改原始碼的提交都要先跑 `scripts/run-tests.sh` 並全數通過**，提交訊息附上結果摘要。
5. **Qt 6 遷移要真正改寫 API**（例如 `QRegExp` → `QRegularExpression`），不要用 `Qt5Compat` 模組湊合過去。改用 `QRegularExpression` 時要確認比對語意相同（完整比對要加錨點）。
6. **授權**：保留所有原始版權聲明與 BSD 條文，不要刪改檔頭。新檔案加上 BSD 條文與新的版權行。不得以 James Larus 的名義替本專案背書或宣傳；README 要明確標示「非官方 fork」。
7. **機密資料**：簽章憑證、Apple ID、App 專用密碼、notarytool 憑證絕不進 repo，也不能出現在日誌或 CI 輸出中。
8. **git**：
   - 不要改寫歷史，不要 force push 到 `main`（除非擁有者明確要求）
   - 不要刪除或修改既有提交中的 `git-svn-id`
   - 這是個人專案：**直接在 `main` 上提交，不要開分支或 PR**（擁有者 2026-10-12 決定）；一個提交只做一件事
   - 提交訊息**不要加 AI 署名**（`Co-Authored-By: Claude…`、`Claude-Session: …` 等）
9. **不要把 repo 改成公開**，也不要推 release tag，這兩件事由擁有者決定。
10. **第三方素材**（圖示、字型）必須是可隨開源軟體散布的授權，並在 README 註明來源。不要打包 Apple 的 SF 字型或 SF Symbols 圖檔。

## 程式風格

- C/C++ 遵循 repo 根目錄的 `.clang-format`（Google style、`SortIncludes: Never`）。只格式化你修改到的程式碼，不要整檔重排。
- 新增的介面程式碼放在 `QtSpim/`，以 Qt Widgets 為主，除非擁有者同意，不要改用 Qt Quick/QML 或 SwiftUI。
- 介面要有**繁體中文**（擁有者 2026-10-12 決定）：原始字串用英文並包在 `tr()` 中，同時更新 `translations/qtspim_zh_TW.ts`；預設跟隨系統語言。MIPS 術語維持英文，用語對照見 `docs/UI_DESIGN.md` 6.1 節。
- `QtSpim/` 等從 SVN 匯入的檔案多為 CRLF 換行，修改時保持原樣（`git diff --stat` 應只顯示改到的行）。

## 介面與編輯器方向（階段 4、5）

- 開始實作前，先提交設計文件（`docs/EDITOR_DESIGN.md`、`docs/UI_DESIGN.md`，介面需附設計稿），**等擁有者核准**。
- 風格：Apple Human Interface Guidelines、現代、科技、簡約。系統字型、等寬字型用 SF Mono（退回 Menlo）、完整淺色 / 深色模式、跟隨系統強調色、大量留白、細分隔線、克制的動畫，並尊重「減少動態效果」設定。
- 編輯器的「組譯並執行」要沿用既有的載入流程（先寫檔再載入），不要為了編輯器修改 `CPU/`。

## 需要停下來詢問擁有者的情況

- 需要修改 `CPU/` 的行為，或測試輸出和原版不一致而找不到原因
- 要刪除檔案（包括 `QtSpim.pro`、舊打包設定）
- 要清理歷史中的大檔 `PCSpim/pcspim.sdf`
- 變更已決定的 Bundle ID、App 名稱、版本號規則、最低支援 macOS 版本（見 `docs/PORTING_PLAN.md`）
- 介面技術路線要偏離 Qt Widgets
- 需要簽章憑證或 Apple 帳號相關資訊
