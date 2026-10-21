# pmaports for the Samsung Galaxy Tab A7 10.4 2020 (gta4l, SM-T500/T505)

postmarketOS packages for gta4l, one folder per package (the same layout as
[LinuxForGta4l/postmarketos-packages-gta4l](https://github.com/LinuxForGta4l/postmarketos-packages-gta4l)).

## Install the prebuilt image

The [Releases](https://github.com/limonabisi/pmaports-gta4l/releases) page has a
ready postmarketOS (Plasma) image for the SM-T500 (Wi-Fi): `gta4l-pmos.tar.xz`
holds the whole system, U-Boot with the kernel, initramfs and device tree
(`boot`), `dtbo`, and postmarketOS on `userdata`. The `cache` partition is not
used.

**This erases everything on the tablet.** You need an unlocked bootloader:
Settings → Developer options → OEM unlocking, then in Download mode hold
Volume Up to unlock.

1. Download `gta4l-pmos.tar.xz` and unpack it into `gta4l-pmos.tar`
   (7-Zip on Windows, `xz -dk gta4l-pmos.tar.xz` on Linux).
2. Power the tablet off, hold Volume Up + Volume Down and plug in the USB
   cable: Download mode.
3. In Odin, put `gta4l-pmos.tar` in **AP** and press Start.
4. The first boot takes a few minutes while the root filesystem grows to fill
   `userdata`.

Log in as **`user`**, password **`147147`**.

> ⚠️ SSH is **on** in this image, and `147147` is public: anyone on the same
> network can log in until you change it. Change the password first thing:
> open Konsole and run `passwd`.

Over the USB cable the tablet is `user@172.16.42.1`.

Updates: `sudo apk add <package>` for new software. The kernel in use is the
one inside the `boot` image.

## Building

Copy the package folders into `device/testing/` of your pmaports checkout
(`pmbootstrap config aports` shows where it is), then:

```sh
pmbootstrap init        # vendor: samsung, device: gta4l, UI: plasma-desktop
pmbootstrap install
pmbootstrap export      # boot.img and the rootfs image
```

The `sha512sums` are filled in. If you change a file, regenerate them with
`pmbootstrap checksum <package>`.

The tablet boots through U-Boot: build it from `u-boot-gta4l/` (see its
README) and wrap the pmOS `boot.img` with `UBOOT-BOOTIMG.py`, which also
writes the Odin tar to flash as AP. The ready U-Boot source is
[limonabisi/u-boot-gta4l](https://github.com/limonabisi/u-boot-gta4l).

The `cache` partition is not needed: remove the `/boot` line from
`/etc/fstab` of the installed rootfs, the kernel comes from the boot image.

## Packages

| Package | What it is |
|---|---|
| `linux-postmarketos-qcom-sm6115` | Kernel 7.2: [LinuxForGta4l/linux](https://github.com/LinuxForGta4l/linux) `gta4l-7.2` at `c91d0e9` plus 11 patches, one per driver and one for the DTS. The same series is the `gta4l-7.2-clean` branch of [limonabisi/linux-gta4l](https://github.com/limonabisi/linux-gta4l/tree/gta4l-7.2-clean). |
| `device-samsung-gta4l` | deviceinfo, firmware import from the tablet's own partitions (read with mtools, never mounted), speaker tuning import, ALSA UCM, wake-up source rule, PAM fix for the systemd user manager, battery count kept across reboots; lighter defaults (no accessibility bus, KWallet or X11 tray bridges, 64 MB journal, Wi-Fi power saving off, weekly TRIM). |
| `device-samsung-gta4l-sensors` | Accelerometer, light sensor and compass through the ADSP (hexagonrpcd + libssc + iio-sensor-proxy). |
| `hexagonrpcd-gta4l` | hexagonrpcd with one patch: it also serves `sns_reg_version` next to the sensor registry. |
| `firmware-samsung-gta4l` | Firmware packages. |
| `u-boot-gta4l/` | U-Boot patches (charging in U-Boot, charge symbol and percentage, ABL command line pass-through) and `UBOOT-BOOTIMG.py`. |

## Status

Working: display, touch (NT36523 lide-hsd), Wi-Fi, Bluetooth, speakers,
microphone, headset jack switching, GPU (Adreno 610, Turnip), battery,
charging (the PMI632 charger watchdog is kept by the kernel), offline
charging (in U-Boot, with the percentage), Bluetooth RFCOMM/BNEP, sleep
(s2idle), accelerometer/light/compass, Venus video decoding (1080p H.264
about 57-97 fps, HEVC about 64 fps, the same frames as software decoding),
USB OTG at 1.5 A, Ethernet over USB, U-Boot.

Known problems:
- Venus encoding and VP9 decoding are off: the Samsung firmware (6.0.49) predates the fixes of 6.0.55. H.264 and HEVC decoding work.
- USB OTG and charging at the same time is not possible: the PMI632 has no
  USB PD PHY, so a hub cannot swap the power role.
- Headphones: only the left channel plays; the right channel's data does not
  reach the WCD937x (its HPHR PDM watchdog trips).
- The ADSP crashes on resume from s2idle while the sensor domain is in use;
  `gta4l-sensors-sleep.service` stops the sensor services around sleep.
- Battery level: the coulomb count now follows the voltage when they disagree
  by more than 25 points on battery; still being verified on the tablet.
- The sensor configuration and registry come from a Galaxy Tab A7; other
  units may need their own calibration.

| Feature | Status | Notes |
|---|:---:|---|
| Boot (U-Boot + kernel 7.2) | ✅ | Menu: Boot Linux, fastboot, recovery, reboot, power off |
| Display | ✅ | 1200x2000, brightness control |
| Touchscreen | ✅ | NT36523 (lide-hsd) tested; firmware of every panel variant included |
| GPU acceleration | ✅ | Adreno 610: freedreno (OpenGL ES 3.2 / GL 4.6), Turnip (Vulkan) |
| Plasma (Wayland) | ✅ | On-screen keyboard: plasma-keyboard |
| Wi-Fi | ✅ | WCN3990, power saving off |
| Bluetooth | ✅ | RFCOMM and BNEP built |
| Speakers | ✅ | FS18xx amplifiers |
| Microphone | ✅ | |
| Headphones | ⚠️ | Only the left channel plays |
| Accelerometer / rotation | ✅ | Through the ADSP (libssc) |
| Ambient light | ✅ | |
| Magnetometer | ✅ | |
| Hall effect (cover) | ⚠️ | Not used for the lid yet |
| Battery level | ⚠️ | The percentage can drift |
| Charging | ✅ | Charger watchdog kept by the kernel |
| Offline charging | ✅ | In U-Boot, with the percentage |
| Suspend (s2idle) | ✅ | Sensor services stopped around sleep |
| USB networking | ✅ | Tablet at `172.16.42.1` |
| USB OTG (host) | ✅ | 1.5 A; keyboard, mouse, storage, Ethernet |
| USB OTG while charging | ❌ | No USB PD on the PMI632 |
| Hardware video decode | ⚠️ | H.264, HEVC; VP9 off (firmware crash) |
| Hardware video encode | ❌ | Samsung firmware 6.0.49 is too old |
| Camera | ❌ | No driver |
| GPS | ❌ | No position (NMEA times out) |
| Mobile data / calls | — | SM-T500 is Wi-Fi only; SM-T505 untested |
| `cache` partition | — | Not needed |

✅ works · ⚠️ partial · ❌ broken · — not applicable
