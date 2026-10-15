// SPDX-License-Identifier: GPL-2.0-only
/*
 * gta4l-charger: offline charging screen for the Galaxy Tab A7 (gta4l).
 *
 * The bootloader starts Linux with androidboot.mode=charger when the tablet
 * is off and a charger is plugged in. Instead of the full system, this runs:
 * it draws the battery level on the framebuffer, turns the screen off after
 * a while, powers off when the charger is pulled, and boots the full system
 * when the power key is held.
 *
 *   power key short press   screen on (or off, if it is on)
 *   power key held 2 s      reboot into the full system
 *   charger pulled          power off
 *
 * The screen is drawn in landscape, the way the tablet is held. The panel is
 * mounted with its left side up (DTS rotation = <270>); --rotate=0|90|180|270
 * overrides that.
 *
 * Static build: aarch64-linux-gnu-gcc -O2 -static -o gta4l-charger gta4l-charger.c
 */
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/reboot.h>
#include <time.h>
#include <unistd.h>

#define BAT		"/sys/class/power_supply/qcom-qg-battery/"
#define SCREEN_ON_MS	15000
#define HOLD_MS		2000
#define UNPLUG_MS	3000

static uint8_t *fb;
static struct fb_var_screeninfo vi;
static struct fb_fix_screeninfo fi;
static char backlight[256];
static int saved_brightness = -1;
static int rot = -1;		/* degrees, clockwise; -1 = landscape if portrait */
static int lw, lh;		/* size of the screen as drawn */

#define COL_BLACK	0x000000
#define COL_WHITE	0xffffff
#define COL_RED		0xe04040
#define COL_GREEN	0x40d060
#define COL_GRAY	0xd0d0d0

/* 10x16 plug, shown left of the battery while charging */
static const char *const plug[] = {
	"..#....#..",
	"..#....#..",
	"..#....#..",
	"#########.",
	"##########",
	"##########",
	"##########",
	".########.",
	"..######..",
	"....##....",
	"....##....",
	"....##....",
	"....##....",
	"....##....",
	"....##....",
	"....##....",
};

static long now_ms(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1000L + ts.tv_nsec / 1000000;
}

static int read_file(const char *path, char *buf, size_t len)
{
	int fd = open(path, O_RDONLY);
	ssize_t n;

	if (fd < 0)
		return -1;
	n = read(fd, buf, len - 1);
	close(fd);
	if (n < 0)
		return -1;
	buf[n] = 0;
	while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == ' '))
		buf[--n] = 0;
	return 0;
}

static void write_file(const char *path, const char *val)
{
	int fd = open(path, O_WRONLY);

	if (fd < 0)
		return;
	if (write(fd, val, strlen(val)) < 0)
		perror(path);
	close(fd);
}

/* x, y as drawn; the framebuffer is turned by rot */
static void put(int lx, int ly, uint32_t rgb)
{
	uint8_t *p;
	int x, y;

	switch (rot) {
	case 90:
		x = vi.xres - 1 - ly;
		y = lx;
		break;
	case 180:
		x = vi.xres - 1 - lx;
		y = vi.yres - 1 - ly;
		break;
	case 270:
		x = ly;
		y = vi.yres - 1 - lx;
		break;
	default:
		x = lx;
		y = ly;
		break;
	}

	if (x < 0 || y < 0 || x >= (int)vi.xres || y >= (int)vi.yres)
		return;
	p = fb + (y + vi.yoffset) * fi.line_length + (x + vi.xoffset) * (vi.bits_per_pixel / 8);
	if (vi.bits_per_pixel == 32) {
		uint32_t v = (((rgb >> 16) & 0xff) << vi.red.offset) |
			     (((rgb >> 8) & 0xff) << vi.green.offset) |
			     ((rgb & 0xff) << vi.blue.offset);
		memcpy(p, &v, 4);
	} else if (vi.bits_per_pixel == 16) {
		uint16_t v = (((rgb >> 19) & 0x1f) << 11) | (((rgb >> 10) & 0x3f) << 5) |
			     ((rgb >> 3) & 0x1f);
		memcpy(p, &v, 2);
	}
}

static void rect(int x, int y, int w, int h, uint32_t rgb)
{
	for (int j = y; j < y + h; j++)
		for (int i = x; i < x + w; i++)
			put(i, j, rgb);
}

/* Seven segment digits: bits a b c d e f g */
static const uint8_t seg[10] = {
	0x7e, 0x30, 0x6d, 0x79, 0x33, 0x5b, 0x5f, 0x70, 0x7f, 0x7b,
};

static void digit(int x, int y, int s, int d, uint32_t c)
{
	int w = 6 * s, t = s, h = 10 * s;
	uint8_t m = seg[d];

	if (m & 0x40) rect(x + t, y, w - 2 * t, t, c);			/* a */
	if (m & 0x20) rect(x + w - t, y + t, t, h / 2 - t, c);		/* b */
	if (m & 0x10) rect(x + w - t, y + h / 2, t, h / 2 - t, c);	/* c */
	if (m & 0x08) rect(x + t, y + h - t, w - 2 * t, t, c);		/* d */
	if (m & 0x04) rect(x, y + h / 2, t, h / 2 - t, c);		/* e */
	if (m & 0x02) rect(x, y + t, t, h / 2 - t, c);			/* f */
	if (m & 0x01) rect(x + t, y + h / 2 - t / 2, w - 2 * t, t, c);	/* g */
}

static void percent(int x, int y, int s, uint32_t c)
{
	int w = 6 * s, h = 10 * s;

	rect(x, y, 2 * s, 2 * s, c);
	rect(x + w - 2 * s, y + h - 2 * s, 2 * s, 2 * s, c);
	for (int i = 0; i < h; i++)
		rect(x + w - s - i * (w - s) / h, y + i, s, 1, c);
}

/* A lying battery filling from the left, the number under it, both centred */
static void draw(int cap, int charging)
{
	int W = lw, H = lh;
	int bh = H / 5, bw = bh * 5 / 2, b = H / 80, s = H / 60, ps = H / 100;
	int bx = (W - bw) / 2, by = (H - (bh + 8 * b + 10 * s)) / 2;
	uint32_t fill = cap <= 15 ? COL_RED : charging ? COL_GREEN : COL_GRAY;
	char txt[8];
	int n, x;

	rect(0, 0, W, H, COL_BLACK);

	/* Battery body and tip */
	rect(bx, by, bw, b, COL_WHITE);
	rect(bx, by + bh - b, bw, b, COL_WHITE);
	rect(bx, by, b, bh, COL_WHITE);
	rect(bx + bw - b, by, b, bh, COL_WHITE);
	rect(bx + bw, by + bh / 3, 2 * b, bh / 3, COL_WHITE);

	/* Level, from the left */
	n = (bw - 4 * b) * cap / 100;
	rect(bx + 2 * b, by + 2 * b, n, bh - 4 * b, fill);

	/* Plug while charging */
	if (charging)
		for (int j = 0; j < 16; j++)
			for (int i = 0; i < 10; i++)
				if (plug[j][i] == '#')
					rect(bx - 14 * ps + i * ps, by + bh / 2 - 8 * ps + j * ps,
					     ps, ps, COL_WHITE);

	/* Number under it */
	n = snprintf(txt, sizeof(txt), "%d", cap);
	x = (W - (n + 1) * 8 * s) / 2;
	for (int i = 0; i < n; i++, x += 8 * s)
		digit(x, by + bh + 8 * b, s, txt[i] - '0', COL_WHITE);
	percent(x, by + bh + 8 * b, s, COL_WHITE);
}

static void screen(int on)
{
	char buf[16];

	if (!backlight[0])
		return;
	if (!on) {
		char path[300];

		snprintf(path, sizeof(path), "%s/brightness", backlight);
		if (saved_brightness < 0 && !read_file(path, buf, sizeof(buf)))
			saved_brightness = atoi(buf);
		write_file(path, "0");
	} else {
		char path[300];

		snprintf(path, sizeof(path), "%s/brightness", backlight);
		if (saved_brightness <= 0) {
			char mp[300];

			snprintf(mp, sizeof(mp), "%s/max_brightness", backlight);
			saved_brightness = read_file(mp, buf, sizeof(buf)) ? 1000 : atoi(buf) / 2;
		}
		snprintf(buf, sizeof(buf), "%d", saved_brightness);
		write_file(path, buf);
	}
}

static int open_power_key(void)
{
	DIR *d = opendir("/dev/input");
	struct dirent *e;
	int found = -1;

	if (!d)
		return -1;
	while ((e = readdir(d))) {
		unsigned long keys[KEY_MAX / (8 * sizeof(long)) + 1] = { 0 };
		char path[300];
		int fd;

		if (strncmp(e->d_name, "event", 5))
			continue;
		snprintf(path, sizeof(path), "/dev/input/%s", e->d_name);
		fd = open(path, O_RDONLY | O_NONBLOCK);
		if (fd < 0)
			continue;
		if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keys)), keys) >= 0 &&
		    (keys[KEY_POWER / (8 * sizeof(long))] >> (KEY_POWER % (8 * sizeof(long))) & 1)) {
			found = fd;
			break;
		}
		close(fd);
	}
	closedir(d);
	return found;
}

static void find_backlight(void)
{
	DIR *d = opendir("/sys/class/backlight");
	struct dirent *e;

	if (!d)
		return;
	while ((e = readdir(d))) {
		if (e->d_name[0] == '.')
			continue;
		snprintf(backlight, sizeof(backlight), "/sys/class/backlight/%s", e->d_name);
		break;
	}
	closedir(d);
}

static void set_rotation(void)
{
	if (rot < 0)
		rot = vi.xres < vi.yres ? 270 : 0;
	lw = (rot == 90 || rot == 270) ? vi.yres : vi.xres;
	lh = (rot == 90 || rot == 270) ? vi.xres : vi.yres;
}

int main(int argc, char **argv)
{
	long screen_until, unplugged_since = -1, pressed_at = -1, last_draw = 0;
	int fbfd, key, on = 1, last_cap = -1, last_chg = -1;

	if (argc > 1 && !strncmp(argv[1], "--rotate=", 9))
		rot = atoi(argv[1] + 9);

	fbfd = open("/dev/fb0", O_RDWR);
	if (fbfd < 0 || ioctl(fbfd, FBIOGET_VSCREENINFO, &vi) || ioctl(fbfd, FBIOGET_FSCREENINFO, &fi)) {
		perror("fb0");
		return 1;
	}
	set_rotation();
	fb = mmap(NULL, fi.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
	if (fb == MAP_FAILED) {
		perror("mmap");
		return 1;
	}

	/* Keep the text console from drawing over us */
	write_file("/sys/class/graphics/fbcon/cursor_blink", "0");
	write_file("/sys/class/vtconsole/vtcon1/bind", "0");

	find_backlight();
	key = open_power_key();
	screen(1);
	screen_until = now_ms() + SCREEN_ON_MS;

	for (;;) {
		struct pollfd p = { .fd = key, .events = POLLIN };
		char cap_s[16] = "0", st[32] = "";
		long t;
		int cap, chg;

		poll(&p, key >= 0 ? 1 : 0, 500);
		t = now_ms();

		if (key >= 0 && (p.revents & POLLIN)) {
			struct input_event ev;

			while (read(key, &ev, sizeof(ev)) == sizeof(ev)) {
				if (ev.type != EV_KEY || ev.code != KEY_POWER)
					continue;
				if (ev.value == 1) {
					pressed_at = t;
				} else if (ev.value == 0 && pressed_at >= 0) {
					pressed_at = -1;
					on = !on;
					screen(on);
					screen_until = t + SCREEN_ON_MS;
					last_cap = -1;
				}
			}
		}

		if (pressed_at >= 0 && t - pressed_at >= HOLD_MS) {
			/* Held: start the full system */
			rect(0, 0, lw, lh, COL_BLACK);
			sync();
			if (system("systemctl reboot") != 0)
				reboot(RB_AUTOBOOT);
			for (;;)
				pause();
		}

		read_file(BAT "capacity", cap_s, sizeof(cap_s));
		read_file(BAT "status", st, sizeof(st));
		cap = atoi(cap_s);
		chg = !strcmp(st, "Charging");

		if (!strcmp(st, "Discharging")) {
			if (unplugged_since < 0)
				unplugged_since = t;
			else if (t - unplugged_since >= UNPLUG_MS) {
				rect(0, 0, lw, lh, COL_BLACK);
				sync();
				if (system("systemctl poweroff") != 0)
					reboot(RB_POWER_OFF);
				for (;;)
					pause();
			}
		} else {
			unplugged_since = -1;
		}

		if (on && t >= screen_until) {
			on = 0;
			screen(0);
		}

		if (on && (cap != last_cap || chg != last_chg || t - last_draw > 10000)) {
			draw(cap, chg);
			last_cap = cap;
			last_chg = chg;
			last_draw = t;
		}
	}
}
