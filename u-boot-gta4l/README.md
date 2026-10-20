# U-Boot for gta4l

Patches on top of [LinuxForGta4l/u-boot](https://github.com/LinuxForGta4l/u-boot)
branch `gta4l` at `0824f9a3`:

- `0001`: keeps the PMI632 charging while U-Boot runs (pets the charger
  watchdog, re-enables charging after a bite, 1.5 A for wall chargers), draws a
  battery symbol in the top right corner while a charger is plugged in (bolt =
  charging, filled = full, yellow = stopped), hands ABL's whole command line to
  Linux (panel name, `androidboot.mode`, `pmos_*`), boots pmOS after 5 s in the
  menu and goes straight to Linux when started by a charger (offline charging).
  Adds the SM4250 msm-id and the novatek/focaltech touch stubs ABL looks up.
- `0002`: drops the old SD-card bootargs (`nr_cpus=1`, `pmos.root=/dev/mmcblk1p1`)
  from the DTS and keeps repeated options such as `console=`.
- `0003`: draws the charger state as a battery of four bars instead: a bolt
  and three bars while charging, four bars when full, "!" and one bar when
  charging stopped.
- `0004`: shows the charge percentage ("NN%", from the PMI632 QGauge voltage)
  next to the battery, the bars follow it; started by a charger, the tablet
  stays in the boot menu and keeps charging instead of booting Linux.

Build:

```sh
git clone -b gta4l https://github.com/LinuxForGta4l/u-boot && cd u-boot
git checkout 0824f9a3 && git am /path/to/u-boot-gta4l/*.patch
make O=out ARCH=arm CROSS_COMPILE=aarch64-linux-gnu- qcom_defconfig
./scripts/config --file out/.config -d TOOLS_MKEFICAPSULE
make O=out ARCH=arm CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc)
gzip -9 -n -c out/u-boot-nodtb.bin > u-boot-nodtb.bin.gz
```

Boot image: `UBOOT-BOOTIMG.py` takes a working pmOS `boot.img` (Image.gz,
initramfs and DTB inside) and makes a boot image whose kernel is U-Boot and whose
ramdisk is a FIT with the Linux kernel, initramfs and DTB, plus an Odin tar.
Python standard library only:

```sh
python3 UBOOT-BOOTIMG.py boot.img u-boot-nodtb.bin.gz \
    out/dts/upstream/src/arm64/qcom/sm6115-samsung-gta4l.dtb gta4l-uboot-boot.img
```

Tested on an SM-T500 (nt36523_lide_hsd panel): boots, all 8 CPUs, panel name
and pmOS options reach Linux.
