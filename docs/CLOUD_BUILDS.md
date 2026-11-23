# The Silicon Preservation Society — 9-Repository Topology & Cloud Build Guide

All **9 repositories** in the **[AX] Ecosystem** are wired with standardized, on-demand GitHub Actions release workflows (`.github/workflows/build-release.yml`, plus `ci.yml` on `Piccolo-2go`). Normal `git push` CI runs stay fast (`~10–15s`), while heavy Buildroot OS images, SDKs, and Skylake/AVX2 emulator binaries compile in the cloud only when triggered.

---

## 1. Ecosystem Architecture & All 9 Repositories

| # | Repository | Build Type | Primary Role | Cloud Workflow | Artifact Name(s) |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **1** | **`ax-sdk`** | CMake (`C++20`) | Shared deterministic runtime (`ax::core`): `2:3:1` & `1:2:1` topology, `SCHED_FIFO` audio ring, HugeTLB fastmem, and `EvdevNavReader` gamepad input. | `build-release.yml` | `ax-sdk-linux-x86_64`<br>`ax-sdk-windows-x86_64` |
| **2** | **`FLAC_AX`** | CMake (`C++20`) | Bit-perfect `S32_LE` / `DoP` audiophile transport (`ES9281PRO` / `ES9281AC Pro`), `libopenmpt` tracker player, `ax_av_studio`, `ax_video_upscaler`, and `ax_appliance_tui`. | `build-release.yml` | `flac-ax-linux-x86_64`<br>`flac-ax-windows-x86_64` |
| **3** | **`Amiberry-AX`** | CMake (`C++20`) | Deterministic Commodore Amiga 500 / 1200 / CD32 engine (`v8.6.0-AX`) linked against `ax-sdk`. | `build-release.yml` | `amiberry-ax-linux-x86_64` |
| **4** | **`Generation-X-AX`** | CMake (`C++20`) | Multi-engine `[AX]` suite powering **Hatari** (Atari ST), **BlastEm** (Mega Drive), **Flycast** (Dreamcast/NAOMI), **RMG** (N64), **Cannonball**, **EDuke32**, and **Frontier**. | `build-release.yml` | `generation-x-ax-linux-x86_64` |
| **5** | **`Eden-Espresso-Wave-AX`** | CMake (`C++20`) | Deterministic Nintendo Switch `[AX]` engine (`clang-cl` Skylake/BMI2, ThinLTO, Elastic `2:3:1` isolation, Intel QSV offload). | `build-release.yml` | `eden-espresso-wave-ax-linux-x86_64`<br>`eden-espresso-wave-ax-windows-x86_64` |
| **6** | **`Cemu-Latte-AX`** | CMake (`C++20`) | Deterministic Nintendo Wii U (Latte GPU) `[AX]` engine linked against `ax-sdk`. | `build-release.yml` | `cemu-latte-ax-linux-x86_64`<br>`cemu-latte-ax-windows-x86_64` |
| **7** | **`Piccolo-2go`** | Buildroot / Shell | Deterministic `aarch64` handheld OS (`128 KB` `zstd` SquashFS `< 140 MB`) for **BATLEXP G350** & **R36S** (`RK3326`, `1:2:1` isolation, `1024 MB LZ4` ZRAM, `Panfrost` KMS/DRM, **HiBy FC3 `1.3 Vrms`** OTG audio). | `ci.yml` *(dispatch)* | `piccolo-2go-rk3326-sdcard` (`sdcard.img`) |
| **8** | **`Piccolo-AX`** | Buildroot / Shell | Bare-metal `x86_64` Batocera Linux console appliance (`Linux 6.12-rt`, `< 300 MB` `toram` EROFS, `2:3:1` isolation, direct ALSA audio). | `build-release.yml` | `piccolo-ax-image-x86_64` |
| **9** | **`piccolo-ax-desktop`** | Buildroot / Shell | On-demand, local-first LXQt/X11 desktop overlay (`< 42 MB` Tier A budget) isolated on Core `5` for `Piccolo-AX`. | `build-release.yml` | `piccolo-ax-desktop-x86_64` |

---

## 2. How to Trigger & Download Remote Builds

### Option A: GitHub Web UI
1. Open `https://github.com<REPO_NAME>` and click **Actions**.
2. Click **Build Release Artifacts** (or **Validate Piccolo-2go External Tree** on `Piccolo-2go`) in the left sidebar.
3. Click **Run workflow** on the right, keep the default branch (`main` / `master`), and click the green **Run workflow** button.
4. When finished, open the completed run and download the `.zip` under **Artifacts** at the bottom of the page (retained for **14 days**).

### Option B: GitHub CLI (`gh`) from Windows PowerShell or Linux
```bash
# 1. Handheld OS image (BATLEXP G350 / R36S)
gh workflow run ci.yml --repo galaxywarriordad/Piccolo-2go --ref main

# 2. Desktop bare-metal OS image (x86_64)
gh workflow run build-release.yml --repo galaxywarriordad/Piccolo-AX --ref main

# 3. Desktop LXQt/X11 overlay (< 42 MB)
gh workflow run build-release.yml --repo galaxywarriordad/piccolo-ax-desktop --ref main -f profile=core

# 4. Bit-perfect audio player, A/V studio & Espresso TUI (Linux + Windows)
gh workflow run build-release.yml --repo galaxywarriordad/FLAC_AX --ref main

# 5. Shared C++20 runtime SDK (Linux + Windows)
gh workflow run build-release.yml --repo galaxywarriordad/ax-sdk --ref main

# 6. Amiga 500/1200/CD32 engine (Linux)
gh workflow run build-release.yml --repo galaxywarriordad/Amiberry-AX --ref master

# 7. Atari ST / Mega Drive / Dreamcast / N64 suite
gh workflow run build-release.yml --repo galaxywarriordad/Generation-X-AX --ref main

# 8. Nintendo Switch [AX] engine (Linux + Windows)
gh workflow run build-release.yml --repo galaxywarriordad/Eden-Espresso-Wave-AX --ref master

# 9. Nintendo Wii U [AX] engine (Linux + Windows)
gh workflow run build-release.yml --repo galaxywarriordad/Cemu-Latte-AX --ref main

# Watch progress & download any completed artifact:
gh run watch --repo galaxywarriordad/Piccolo-2go
gh run download --repo galaxywarriordad/Piccolo-2go -n piccolo-2go-rk3326-sdcard -D ./downloads
```

---

## 3. Flashing & Deployment Notes
* **`Piccolo-2go` (`sdcard.img`):** Flash to your **`64 GB` SanDisk Extreme PRO (`TF1`)** using **Rufus** or **Win32DiskImager** *(never Balena Etcher on RK3326 images, as it can misalign raw `32 KB` / `8 MB` U-Boot sectors)*. Format your **`128 GB` SanDisk Extreme PRO (`TF2`)** as `exFAT` with `/roms/fbneo/` and `/music/`.
* **`Piccolo-AX`:** Flash to a USB 3.0 drive with **Rufus** or `dd` and boot with `toram` enabled in GRUB.
