# QtSpim ARM 移植任務說明（交接用）

## 背景

- QtSpim 是 James R. Larus 開發的 MIPS32 模擬器，BSD 3-Clause 授權。官方 macOS 版只有 Intel 版，需要 Rosetta 2；而且沒有經過簽章與公證，Homebrew 已在 2026-09-01 停用 `qtspim` cask。
- 本專案目標：
  1. 把 QtSpim 移植成 **macOS 原生 Universal Binary（arm64 + x86_64）**，用 Apple Developer ID 簽章並公證後開源發布。
  2. 加入**內建程式編輯器**，讓使用者可以在軟體內撰寫、修改組合語言程式並直接執行。
  3. 介面改為**現代、科技、簡約的 Apple 風格**。
- 順序：先完成移植並確認行為一致（階段 0–3），再做編輯器和介面（階段 4–5）。不要同時進行，否則出問題時無法分辨是移植還是新功能造成的。
- 擁有者：tw-yuan（具 Apple Developer Program 會員資格）

## 目前狀態

- Repo：`https://github.com/tw-yuan/qtspim-arm`（**私人**），預設分支 `main`
- 內容：從 SourceForge SVN `https://svn.code.sf.net/p/spimsimulator/code` 以 `git svn` 完整轉入，共 764 個提交（r1–r764，最後一筆 2023-08-04）。每個提交訊息都保留 `git-svn-id`。
- 目錄重點：
  - `CPU/`：模擬核心（C/C++、`parser.y`、`scanner.l`、`exceptions.s`）
  - `QtSpim/`：Qt 圖形介面，建置檔為 `QtSpim.pro`（qmake），使用 `core widgets printsupport`
  - `spim/`：命令列版，`Makefile` 內有 `test`、`test_bare` 目標
  - `Tests/`：原作者的 torture test（`tt.bare.s`、`tt.core.s`、`tt.le.s`、`tt.dir.s`、`tt.alu.bare.s`、`tt.fpu.bare.s`、`tt.in` 等）
  - `Setup/QtSpim_Mac_Deployment/`：舊的 Packages（.pkgproj）打包設定
  - `PCSpim/`、`xspim/`、Windows 相關：本次不處理
- 已知 Qt 5 專屬 API（只會在 Qt 6 出問題的部分）：`QRegExp` 9 處、`QDesktopWidget` 1 處（位於 `QtSpim/*.cpp|*.h`）
- 說明文件建置使用 `qcollectiongenerator`，新版 Qt 已移除，需改用 `qhelpgenerator`
- 歷史中有一個 51 MB 的 `PCSpim/pcspim.sdf`（VS IntelliSense 快取）。**未經擁有者同意不要改寫歷史。**

## 硬性規則

1. **行為必須和原版一致。** 這是教學用模擬器，學生作業由助教用原版 SPIM 評分；任何指令語意、syscall、例外處理、延遲槽、記憶體配置、輸出格式的改變都視為 bug。
2. **`CPU/` 模擬核心盡量不動。** 只允許為了能在 arm64 / 新編譯器上編譯所需的最小修改，每一處都要在提交訊息中說明原因。
3. **授權**：保留所有原始版權聲明與 BSD 條文；新檔案加上原 BSD 條文加上新的版權行。不得以 James Larus 的名義替本專案背書或宣傳。Qt 為 LGPL，打包時需附授權聲明並維持動態連結。
4. **不要使用 `Qt5Compat` 模組**湊合過去，要真正遷移到 Qt 6 API。
5. **機密資料**：簽章憑證、Apple ID、App 專用密碼、notarytool 憑證**絕不進 repo**，也不要寫進任何日誌。只能用 GitHub Actions secrets 或本機鑰匙圈。
6. **不要改寫 git 歷史**、不要 force push 到 `main`。每個階段開一個分支並發 PR。
7. **不要把 repo 改成公開**，由擁有者決定時機。

## 任務（依序進行，每階段有驗收條件）

### 階段 0：開發環境

- 平台：macOS（Apple Silicon）
- 安裝：`brew install qt cmake bison flex`
- 注意：macOS 內建的 bison 版本非常舊（2.3），Homebrew 的 bison 是 keg-only，建置時要明確使用 `$(brew --prefix bison)/bin/bison`（flex 同理），不要依賴 PATH 上的系統版本。
- **驗收**：記錄所用的 Qt、CMake、bison、flex、Xcode/clang 版本到 `docs/BUILDING.md`

### 階段 1：建立行為基準（最優先，先於任何程式碼修改）

1. 在未修改的程式碼上編譯命令列版 `spim`（`spim/Makefile`），或使用 `brew install spim`（Homebrew 的 spim 9.1.24 正是 SVN r764 編出來的，可作為獨立參考）。
2. 依照 `spim/Makefile` 的 `test` / `test_bare` 目標中的參數，逐一執行 `Tests/` 下的測試，將完整輸出存成 golden files，放在 `tests/golden/`。
3. 另外寫一批補充測試程式放在 `tests/cases/`，至少涵蓋：各 syscall（print/read int、float、double、string、sbrk、exit、exit2）、例外與 `exceptions.s` 處理、分支與載入延遲選項（`-delayed_branches`、`-delayed_loads`）、`-bare` 模式、大小端相關指令、浮點運算。用原版產生預期輸出。
4. 寫 `scripts/run-tests.sh`：用指定的 spim 執行檔跑所有測試並和 golden 比對，有差異就以非零碼結束並顯示 diff。
- **驗收**：在原始碼上執行 `scripts/run-tests.sh` 全數通過；PR 只包含測試和腳本，不含任何原始碼修改。

### 階段 2：建置系統改為 CMake + Qt 6

1. 新增頂層 `CMakeLists.txt`，產出兩個目標：命令列版 `spim` 和圖形介面版 `QtSpim.app`。
2. 用 `FindBISON` / `FindFLEX` 產生 parser/scanner，保留原本的符號前綴設定（bison `-p yy`、flex `-Pyy`，以及 `--defines=parser_yacc.h` 等輸出名稱），讓產生的檔名和符號與 qmake 版一致。
3. Universal Binary：`CMAKE_OSX_ARCHITECTURES="arm64;x86_64"`，最低系統版本依 Qt 6 的需求設定並記錄在 `docs/BUILDING.md`。
4. 資源檔：`windows_images.qrc`、`exception.qrc`、`NewIcon.icns`、`macinfo.plist` 要正確帶入 app bundle。
5. 說明文件：改用 `qhelpgenerator` 產生 `.qch` / `.qhc`。
6. 暫時保留 `QtSpim.pro` 不刪，等擁有者確認後再移除。
- **驗收**：`cmake -B build && cmake --build build` 在 Apple Silicon 上成功；`lipo -info` 顯示兩個架構；用新建置的 `spim` 跑 `scripts/run-tests.sh` 全數通過。

### 階段 3：Qt 6 API 遷移

1. `QRegExp` → `QRegularExpression`（9 處）。注意兩者語法和比對語意的差異（例如 `exactMatch` 要改用錨點或 `match().hasMatch()` 搭配完整比對），逐一確認結果相同。
2. `QDesktopWidget` → `QScreen` / `QGuiApplication::primaryScreen()`（1 處）。
3. 清除其餘 Qt 6 編譯警告與已棄用 API。
4. 確認 Retina 高解析度顯示、深色模式下文字可讀（只修顯示問題，不改版面設計）。
- **驗收**：零錯誤；警告清單附在 PR 中並說明哪些刻意保留；`scripts/run-tests.sh` 全數通過。

### 階段 4：內建程式編輯器

**先提交設計說明（`docs/EDITOR_DESIGN.md`）給擁有者確認，再開始實作。**

功能需求：
1. 以 `QPlainTextEdit` 為基礎的編輯器，可開新檔、開啟、儲存、另存、最近開啟的檔案、未儲存提示。
2. MIPS 組合語言語法高亮：指令、虛擬指令、暫存器（`$t0` 和 `$8` 兩種寫法）、組譯指示（`.data`、`.text`、`.word`、`.asciiz` 等）、標籤、註解、字串、數字（十進位、十六進位）。
3. 行號、目前行高亮、自動縮排、Tab/空白設定、尋找與取代。
4. **組譯並執行**：一鍵將編輯器內容載入模擬器並執行。
   - 為了不動 `CPU/` 核心，做法是先把緩衝區內容寫到暫存檔（或已儲存的檔案），再呼叫既有的載入流程。
5. **錯誤定位**：組譯錯誤在對應行標示（行號旁圖示加底線），點擊錯誤訊息可跳到該行。需要從既有錯誤訊息中解析行號；若核心沒有提供足夠資訊，回報給擁有者，不要自行修改核心的錯誤輸出格式。
6. **執行同步**：單步或停在斷點時，編輯器高亮目前執行到的原始碼行；可在編輯器行號旁點擊設定斷點，並和既有斷點機制同步。
7. 編輯器與既有的文字段 / 資料段 / 暫存器視窗並存，不移除任何既有功能。

驗收：
- 上述功能逐項可用，附操作截圖或短片
- 用編輯器寫的程式和從檔案載入同一程式，執行結果完全相同
- `scripts/run-tests.sh` 仍全數通過（命令列版不受影響）

### 階段 5：介面現代化（Apple 風格：現代、科技、簡約）

**先產出設計稿（mockup 或截圖拼版）與 `docs/UI_DESIGN.md`，經擁有者確認後才實作。**

方向：
1. 遵循 Apple Human Interface Guidelines 的 macOS 慣例：統一標題列與工具列、左側側邊欄（檔案 / 符號 / 斷點）、主區為編輯器與程式碼視圖、右側或下方為暫存器 / 記憶體 / Console 面板，面板可收合與調整大小。
2. 字型：介面用系統字型（San Francisco，透過系統預設字型取得，不要把 SF 字型打包進 app）；程式碼與數值用等寬字型（優先 SF Mono，若取不到則退回 Menlo）。
3. 色彩：完整支援淺色 / 深色模式並跟隨系統設定；強調色跟隨系統 accent color；低飽和、大量留白、細分隔線，避免厚重邊框與漸層。
4. 暫存器 / 記憶體視圖：數值變動時短暫高亮，可切換十進位 / 十六進位 / 二進位，等寬對齊。
5. 圖示：使用授權允許隨開源軟體散布的圖示集（例如 Lucide，ISC 授權），並在 README 註明來源。**不要直接擷取或打包 Apple SF Symbols 圖檔**，其授權限制較多，若擁有者希望使用需先確認授權。
6. 動畫：只用短而克制的過場（面板收合、高亮淡出），並尊重系統「減少動態效果」設定。
7. 可用性：鍵盤快捷鍵符合 macOS 慣例（⌘R 執行、⌘B 組譯、F10/F11 單步等，需在設計文件列出完整對照）；支援 VoiceOver 基本標籤；文字對比符合 WCAG AA。
8. 實作方式預設維持 **Qt Widgets**（搭配原生 macOS style 與少量 QSS），不要在未經同意下改寫成 Qt Quick/QML 或 SwiftUI。

驗收：
- 淺色 / 深色兩種模式截圖，和設計稿一致
- 既有功能全部仍可使用（以階段 6 檢查表驗證）
- `scripts/run-tests.sh` 仍全數通過

### 階段 6：圖形介面手動驗證

在 arm64 與 x86_64（Rosetta 執行 Universal 版的 x86_64 部分：`arch -x86_64`）各跑一次，逐項打勾並附截圖：
- 載入 `.s` 檔、重新載入、清除
- 執行、單步、從斷點繼續、設定/移除斷點
- 暫存器視窗（整數、浮點）、數值顯示格式切換
- 文字段、資料段視窗正確顯示
- Console 輸入輸出（含 read 類 syscall）
- 設定（例外處理檔、bare 模式、延遲選項等）可儲存並在重啟後保留
- 說明視窗能開啟
- 列印 / 存日誌功能
- 編輯器：新建、編輯、儲存、組譯並執行、錯誤跳行、執行行同步、編輯器斷點
- 淺色 / 深色模式切換、視窗縮放、面板收合
- **驗收**：`docs/MANUAL_TEST.md` 填好檢查表與結果。

### 階段 7：打包、簽章、公證（需要擁有者提供憑證，代理人不得自行取得）

1. `macdeployqt` 打包 Qt framework 進 app bundle。
2. 以 **Developer ID Application** 憑證簽章，開啟 Hardened Runtime，entitlements 只放必要項目（預設應為空）。
3. 製作 DMG，簽章後用 `xcrun notarytool submit --wait` 公證，再 `xcrun stapler staple`。
4. 寫 `scripts/package-macos.sh` 完成上述流程，憑證透過環境變數或鑰匙圈 profile 名稱傳入。
- **驗收**：在一台乾淨的 Mac 上從網路下載 DMG 後可直接開啟，不出現 Gatekeeper 警告；`spctl -a -vv -t install` 與 `spctl -a -vv QtSpim.app` 都顯示 accepted 且來源為 Notarized Developer ID；`codesign --verify --deep --strict` 通過。

### 階段 8：CI

1. `.github/workflows/ci.yml`：PR 與 push 時在 macOS arm64 runner 上建置並執行 `scripts/run-tests.sh`。
2. `.github/workflows/release.yml`：推送 `v*` tag 時建置、簽章、公證並建立 GitHub Release（附 DMG 與 SHA-256）。
3. 簽章相關 secrets 只開放給 release workflow，並建議擁有者設定 tag protection / environment 審核，避免任何 PR 取得簽章能力。
- **驗收**：CI 在 PR 上自動跑且通過；release workflow 寫好但先不要真的推 tag，由擁有者觸發第一次發布。

### 階段 9：文件

- 新的 `README.md`：
  - 開頭明確註明「非官方 fork，與原作者無關，未經原作者背書」
  - 和原版的差異（原生 Apple Silicon、Universal、已簽章公證、Qt 6、內建編輯器、新介面）
  - 介面截圖（淺色 / 深色）
  - 第三方素材（圖示集等）的授權來源
  - 安裝與從原始碼建置方式
  - 完整保留原作者的版權聲明與 BSD 條文，以及 Qt LGPL 說明
- 原本的 `README` 保留不動（可改名為 `README.upstream` 前先徵求擁有者同意）
- `CHANGES.md`：列出本 fork 的所有修改
- **驗收**：擁有者審閱通過。

## 不在本次範圍

- PCSpim、xspim、Windows / Linux 安裝程式（不要刪除，也不要修改）
- 階段 4、5 以外的新功能
- 模擬核心重構
- Homebrew cask / tap 提交（等擁有者決定發布策略後再做）

## 需要擁有者決定的事項（遇到時停下來詢問，不要自行決定）

1. 是否改寫歷史移除 `PCSpim/pcspim.sdf`（會改變所有提交 hash）
2. 是否刪除 `QtSpim.pro` 及其他 qmake 相關檔案
3. Bundle ID、App 顯示名稱、版本號規則（建議和上游 9.1.24 區隔，例如 `9.1.24-arm.1`）
4. 最低支援的 macOS 版本
5. 是否先聯絡原作者尋求認可（會影響 Homebrew 上架方式與命名）
6. Repo 何時公開
7. 編輯器設計說明與介面設計稿是否核准（階段 4、5 開工前）
8. 介面技術路線：維持 Qt Widgets（預設），或改用 Qt Quick/QML、SwiftUI（後兩者工程量大很多，SwiftUI 還會失去跨平台能力）
9. 是否先發布「純移植版」（階段 0–3 + 簽章）再做編輯器與新介面，或全部完成後一次發布

## 回報方式

- 每完成一個階段：開一個 PR，說明做了什麼、驗收結果（附指令輸出）、遇到的問題與未解決事項。
- 測試若出現和原版不一致的輸出，**不要修改 golden file 讓它通過**，要找出原因並回報。

## 擁有者決定紀錄（2026-10-12）

上方「需要擁有者決定的事項」的決定如下，以及移植過程中另外決定的事項：

| 事項 | 決定 |
|---|---|
| 1. 改寫歷史移除 `PCSpim/pcspim.sdf` | **不改寫**（打包後約 16 MiB，r469 起已不在檔案樹中） |
| 2. 刪除 `QtSpim.pro` | CMake 在 Mac 與 CI 驗證通過、第一版發布後，另開 PR 刪除；`spim/Makefile` **保留** |
| 3. Bundle ID | `net.yuan-tw.qtspim` |
| 3. App 顯示名稱 | `QtSpim ARM` |
| 3. 版本號 | release tag `v9.1.24-arm.N`；`CFBundleShortVersionString` = `9.1.24`；`CFBundleVersion` = 遞增的建置編號（1、2、3…）；命令列版 `spim` 的版本字串（`CPU/version.h`）不動 |
| 4. 最低 macOS | macOS 13（Qt 6.11 的需求；原定 Qt 6.8 / macOS 12，見下方 Qt 版本） |
| 5. 聯絡原作者 | 擁有者自行處理 |
| 6. Repo 公開 | 盡快，`README.md` 標示非官方 fork 後即公開 |
| 7. 編輯器、介面設計稿 | 尚未提交；第一版發布後才開始 |
| 8. 介面技術路線 | Qt Widgets |
| 9. 發布策略 | 先發布純移植版（階段 0–3＋簽章、CI、README） |
| arm64 浮點差異 | 在 `CPU/run.cpp` 做最小修正，所有主機都產生 x86 的結果（無效運算的 NaN、超出範圍的浮點轉整數） |
| Qt 版本 | 正式版用 **Qt 6.11** 的 Universal 版（官方安裝程式或 aqtinstall）；Homebrew 的 Qt 只用於 arm64 開發。原定 Qt 6.8 LTS，但開源版 6.8 最後一版 6.8.3 無法用 macOS 26 以後的 SDK 連結（AGL），且不再更新，擁有者於同日改為 Qt 6.11 |
| 新檔案的版權行 | `Copyright (c) 2026, qtspim-arm contributors.` |
| 說明視窗 | 把 Qt Assistant 打包進 app，說明檔路徑改成相對於 app 本身（不再寫死 `/Applications/QtSpim.app`） |
| `CLAUDE.md`、本文件 | 提交進 repo（本文件原為 `TO_DO.md`） |
