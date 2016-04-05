/*
 *  nintendo3ds_input.c
 *
 *  Copyright (C) 2015 Sergi Granell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/input.h>
#include <linux/input-polldev.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/of.h>
#include <asm/io.h>

#include <mach/platform.h>
#include <mach/bottom_lcd.h>

/***** Virtual keyboard *****/

enum vkb_mode_t {
	VKB_MODE_KEYBOARD,
	VKB_MODE_MOUSE,
	VKB_MODE_ARROW, // switch TTY
};

struct vkb_ctx_t {
	const struct font_desc *font;
	enum vkb_mode_t mode;
	unsigned int old_buttons;
	int curx;
	int cury;
	int tty;
};

typedef void (*vkb_input_poll_cb_t)(struct vkb_ctx_t *vkb, int buttons, struct input_dev *idev);

static void vkb_input_poll_cb_keyboard(struct vkb_ctx_t *vkb, int buttons, struct input_dev *idev);
static void vkb_input_poll_cb_mouse(struct vkb_ctx_t *vkb, int buttons, struct input_dev *idev);
static void vkb_input_poll_cb_arrow(struct vkb_ctx_t *vkb, int buttons, struct input_dev *idev);

static const vkb_input_poll_cb_t vkb_input_poll_cbs[] = {
	vkb_input_poll_cb_keyboard,
	vkb_input_poll_cb_mouse,
	vkb_input_poll_cb_arrow
};

static const char *vkb_input_mode_str[] = {
	"keyboard",
	"mouse",
	"arrows/change tty"
};

#define VKB_NUM_MODES (sizeof(vkb_input_poll_cbs)/sizeof(*vkb_input_poll_cbs))

#define VKB_ROWS (5)
#define VKB_COLS (14)

static const char *vkb_map_ascii[VKB_ROWS][VKB_COLS] = {
	{ "`   ", "1",     "2",    "3",  "4",  "5",  "6",  "7",  "8",  "9",  "0",  "-",   "=",    "Del" },
	{ "TAB ", "Q",     "W",    "E",  "R",  "T",  "Y",  "U",  "I",  "O",  "P",  "{",   "}",     "\\" },
	{ "CAPS", "A",     "S",    "D",  "F",  "G",  "H",  "J",  "K",  "L",  ":",  "´",   "Enter", NULL },
	{ "SHFT", "Z",     "X",    "C",  "V",  "B",  "N",  "M",  ",",  ".",  "/",  "SHFT", NULL,   NULL },
	{ "CTRL", "SPACE", "CTRL", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,   NULL,   NULL }
};

static const char vkb_map_keys[VKB_ROWS][VKB_COLS] = {
	{ KEY_GRAVE,     KEY_1,     KEY_2,         KEY_3,  KEY_4, KEY_5, KEY_6, KEY_7, KEY_8,     KEY_9,   KEY_0,         KEY_MINUS,      KEY_EQUAL,      KEY_BACKSPACE },
	{ KEY_TAB,       KEY_Q,     KEY_W,         KEY_E,  KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I,     KEY_O,   KEY_P,         KEY_LEFTBRACE,  KEY_RIGHTBRACE, KEY_BACKSLASH },
	{ KEY_CAPSLOCK,  KEY_A,     KEY_S,         KEY_D,  KEY_F, KEY_G, KEY_H, KEY_J, KEY_K,     KEY_L,   KEY_SEMICOLON, KEY_APOSTROPHE, KEY_ENTER,      0             },
	{ KEY_LEFTSHIFT, KEY_Z,     KEY_X,         KEY_C,  KEY_V, KEY_B, KEY_N, KEY_M, KEY_COMMA, KEY_DOT, KEY_SLASH,     KEY_RIGHTSHIFT, 0,              0             },
	{ KEY_LEFTCTRL,  KEY_SPACE, KEY_RIGHTCTRL, 0,      0,     0,     0,     0,     0,         0,       0,             0,              0,              0             }
};

static void vkb_draw_bottom_lcd(const struct vkb_ctx_t *vkb, int x, int y)
{
	int i, j;
	int dx = x;
	int dy = y;

	for (i = 0; i < VKB_ROWS; i++) {
		for (j = 0; j < VKB_COLS; j++) {
			if (vkb_map_ascii[i][j]) {
				dx += nintendo3ds_bottom_lcd_draw_text(vkb->font, dx, dy,
					(vkb->curx == j && vkb->cury == i) ? COLOR_RED : COLOR_WHITE,
					vkb_map_ascii[i][j]) + vkb->font->width;
			}
		}
		dx = x;
		dy += vkb->font->height;
	}

	nintendo3ds_bottom_lcd_draw_fillrect(x, y + (VKB_ROWS + 1)*vkb->font->height,
		300, vkb->font->height, COLOR_BLACK);

	nintendo3ds_bottom_lcd_draw_textf(vkb->font, x, y + (VKB_ROWS + 1)*vkb->font->height,
		COLOR_GREEN, "Input mode: %s", vkb_input_mode_str[vkb->mode]);
}

static int vkb_init(struct vkb_ctx_t *vkb)
{
	vkb->font = get_default_font(NINTENDO3DS_LCD_BOT_WIDTH, NINTENDO3DS_LCD_BOT_HEIGHT, -1, -1);
	vkb->mode = VKB_MODE_KEYBOARD;
	vkb->old_buttons = 0xFFF;
	vkb->curx = VKB_COLS/2;
	vkb->cury = VKB_ROWS/2;
	vkb->tty = 1;

	nintendo3ds_bottom_lcd_clear_screen(COLOR_BLACK);
	vkb_draw_bottom_lcd(vkb, 0, 0);

	return 0;
}

static void vkb_free(struct vkb_ctx_t *vkb)
{
	if (!vkb)
		return;
}


/***** Buttons *****/

/* We poll keys - msecs */
#define POLL_INTERVAL_DEFAULT	20

#define BUTTON_A      (1 << 0)
#define BUTTON_B      (1 << 1)
#define BUTTON_SELECT (1 << 2)
#define BUTTON_START  (1 << 3)
#define BUTTON_RIGHT  (1 << 4)
#define BUTTON_LEFT   (1 << 5)
#define BUTTON_UP     (1 << 6)
#define BUTTON_DOWN   (1 << 7)
#define BUTTON_R1     (1 << 8)
#define BUTTON_L1     (1 << 9)
#define BUTTON_X      (1 << 10)
#define BUTTON_Y      (1 << 11)

#define BUTTON_HELD(b, m) (~(b) & (m))
#define BUTTON_PRESSED(b, o, m) ((~(b) & (o)) & (m))
#define BUTTON_CHANGED(b, o, m) (((b) ^ (o)) & (m))

struct nintendo3ds_input_dev {
	struct input_polled_dev *pdev;
	void __iomem *hid_input;
	struct vkb_ctx_t vkb;
};

#define report_key_button(key, button) \
	do { \
		input_report_key(idev, key, BUTTON_HELD(buttons, button)); \
		input_sync(idev); \
	} while (0)


static void vkb_input_poll_cb_keyboard(struct vkb_ctx_t *vkb, int buttons, struct input_dev *idev)
{
	int old_buttons = vkb->old_buttons;

	/* Crappy code (will change it soon) */
	if (BUTTON_CHANGED(buttons, old_buttons, BUTTON_A))
			report_key_button(vkb_map_keys[vkb->cury][vkb->curx], BUTTON_A);
	else if (BUTTON_CHANGED(buttons, old_buttons, BUTTON_START))
			report_key_button(KEY_ENTER, BUTTON_START);
	else if (BUTTON_CHANGED(buttons, old_buttons, BUTTON_B))
			report_key_button(KEY_BACKSPACE, BUTTON_B);
	else if (BUTTON_CHANGED(buttons, old_buttons, BUTTON_X))
			report_key_button(KEY_SPACE, BUTTON_X);
	else if (BUTTON_CHANGED(buttons, old_buttons, BUTTON_Y)) {
			input_report_key(idev, KEY_LEFTCTRL, BUTTON_HELD(buttons, BUTTON_Y));
			input_report_key(idev, KEY_LEFTALT, BUTTON_HELD(buttons, BUTTON_Y));
			input_report_key(idev, KEY_BACKSPACE, BUTTON_HELD(buttons, BUTTON_Y));
			input_report_key(idev, KEY_C, BUTTON_HELD(buttons, BUTTON_Y));
			input_sync(idev);
	} else if (buttons ^ old_buttons) {
		if (BUTTON_PRESSED(buttons, old_buttons, BUTTON_UP)) {
			if (--vkb->cury < 0)
				vkb->cury = VKB_ROWS - 1;
		} else if (BUTTON_PRESSED(buttons, old_buttons, BUTTON_DOWN)) {
			if (++vkb->cury > VKB_ROWS-1)
				vkb->cury = 0;
		} else if (BUTTON_PRESSED(buttons, old_buttons, BUTTON_LEFT)) {
			if (--vkb->curx < 0)
				vkb->curx = VKB_COLS-1;
		} else if (BUTTON_PRESSED(buttons, old_buttons, BUTTON_RIGHT)) {
			if ((++vkb->curx > VKB_COLS-1) || (vkb_map_keys[vkb->cury][vkb->curx] == 0))
				vkb->curx = 0;
		}
		if (BUTTON_PRESSED(buttons, old_buttons,
		    BUTTON_UP | BUTTON_DOWN | BUTTON_LEFT | BUTTON_RIGHT)) {
			while (vkb_map_keys[vkb->cury][vkb->curx] == 0)
				vkb->curx--;
			vkb_draw_bottom_lcd(vkb, 0, 0);
		}
	}
}

static void vkb_input_poll_cb_mouse(struct vkb_ctx_t *vkb, int buttons, struct input_dev *idev)
{
	int old_buttons = vkb->old_buttons;

	if (BUTTON_CHANGED(buttons, old_buttons, BUTTON_A))
			report_key_button(BTN_LEFT, BUTTON_A);
	else if (BUTTON_CHANGED(buttons, old_buttons, BUTTON_Y))
			report_key_button(BTN_RIGHT, BUTTON_Y);
	else if (BUTTON_CHANGED(buttons, old_buttons, BUTTON_X))
			report_key_button(BTN_MIDDLE, BUTTON_X);

	if (BUTTON_HELD(buttons, BUTTON_UP)) {
		input_report_rel(idev, REL_Y, -5);
		input_sync(idev);
	} else if (BUTTON_HELD(buttons, BUTTON_DOWN)) {
		input_report_rel(idev, REL_Y, +5);
		input_sync(idev);
	} else if (BUTTON_HELD(buttons, BUTTON_LEFT)) {
		input_report_rel(idev, REL_X, -5);
		input_sync(idev);
	} else if (BUTTON_HELD(buttons, BUTTON_RIGHT)) {
		input_report_rel(idev, REL_X, +5);
		input_sync(idev);
	}

}

static void vkb_input_poll_cb_arrow(struct vkb_ctx_t *vkb, int buttons, struct input_dev *idev)
{
	int old_buttons = vkb->old_buttons;

	if (BUTTON_CHANGED(buttons, old_buttons, BUTTON_R1)) {
			if (BUTTON_HELD(buttons, BUTTON_R1))
				if ((++vkb->tty) > 10)
					vkb->tty = 1;
			input_report_key(idev, KEY_LEFTCTRL,
				BUTTON_HELD(buttons, BUTTON_R1));
			input_report_key(idev, KEY_LEFTALT,
				BUTTON_HELD(buttons, BUTTON_R1));
			input_report_key(idev, KEY_F1 + vkb->tty - 1,
				BUTTON_HELD(buttons, BUTTON_R1));
			input_sync(idev);
	} else if (BUTTON_CHANGED(buttons, old_buttons, BUTTON_L1)) {
			if (BUTTON_HELD(buttons, BUTTON_L1))
				if ((--vkb->tty) < 1)
					vkb->tty = 10;
			input_report_key(idev, KEY_LEFTCTRL,
				BUTTON_HELD(buttons, BUTTON_L1));
			input_report_key(idev, KEY_LEFTALT,
				BUTTON_HELD(buttons, BUTTON_L1));
			input_report_key(idev, KEY_F1 + vkb->tty - 1,
				BUTTON_HELD(buttons, BUTTON_L1));
			input_sync(idev);
	} else if (BUTTON_CHANGED(buttons, old_buttons, BUTTON_UP)) {
		report_key_button(KEY_UP, BUTTON_UP);
	} else if (BUTTON_CHANGED(buttons, old_buttons, BUTTON_DOWN)) {
		report_key_button(KEY_DOWN, BUTTON_DOWN);
	} else if (BUTTON_CHANGED(buttons, old_buttons, BUTTON_LEFT)) {
		report_key_button(KEY_LEFT, BUTTON_LEFT);
	} else if (BUTTON_CHANGED(buttons, old_buttons, BUTTON_RIGHT)) {
		report_key_button(KEY_RIGHT, BUTTON_RIGHT);
	} else if (BUTTON_CHANGED(buttons, old_buttons, BUTTON_START)) {
		report_key_button(KEY_ENTER, BUTTON_START);
	}
}

static void nintendo3ds_input_poll(struct input_polled_dev *pdev)
{
	unsigned int buttons;
	struct nintendo3ds_input_dev *n3ds_input_dev = pdev->private;
	struct vkb_ctx_t *vkb = &n3ds_input_dev->vkb;
	struct input_dev *idev = pdev->input;

	buttons = ioread32(n3ds_input_dev->hid_input);

	if (BUTTON_PRESSED(buttons, vkb->old_buttons, BUTTON_SELECT)) {
		vkb->mode = (vkb->mode + 1)%VKB_NUM_MODES;
		vkb_draw_bottom_lcd(vkb, 0, 0);
	}

	vkb_input_poll_cbs[vkb->mode](vkb, buttons, idev);

	vkb->old_buttons = buttons;
}

static int nintendo3ds_input_probe(struct platform_device *plat_dev)
{
	int i;
	int error;
	struct nintendo3ds_input_dev *n3ds_input_dev;
	struct input_polled_dev *pdev;
	struct input_dev *idev;
	void *hid_input;

	n3ds_input_dev = kzalloc(sizeof(*n3ds_input_dev), GFP_KERNEL);
	if (!n3ds_input_dev) {
		error = -ENOMEM;
		goto err_alloc_n3ds_input_dev;
	}

	/* Try to map HID_input */
	if (request_mem_region(NINTENDO3DS_REG_HID, NINTENDO3DS_REG_HID_SIZE, "N3DS_HID_INPUT")) {
		hid_input = ioremap_nocache(NINTENDO3DS_REG_HID, NINTENDO3DS_REG_HID_SIZE);

		printk("HID_INPUT mapped to: %p - %p\n", hid_input,
			hid_input + NINTENDO3DS_REG_HID_SIZE);
	} else {
		printk("HID_INPUT region not available.\n");
		error = -ENOMEM;
		goto err_hidmem;
	}

	pdev = input_allocate_polled_device();
	if (!pdev) {
		printk(KERN_ERR "nintendo3ds_input.c: Not enough memory\n");
		error = -ENOMEM;
		goto err_alloc_pdev;
	}

	pdev->poll = nintendo3ds_input_poll;
	pdev->poll_interval = POLL_INTERVAL_DEFAULT;
	pdev->private = n3ds_input_dev;

	idev = pdev->input;
	idev->name = "Nintendo 3DS input";
	idev->phys = "nintendo3ds/input0";
	idev->id.bustype = BUS_HOST;
	idev->dev.parent = &plat_dev->dev;

	__set_bit(EV_REL, idev->evbit);
	__set_bit(REL_X, idev->relbit);
	__set_bit(REL_Y, idev->relbit);
	__set_bit(REL_WHEEL,idev->relbit);

	__set_bit(EV_KEY, idev->evbit);
	__set_bit(BTN_LEFT, idev->keybit);
	__set_bit(BTN_RIGHT, idev->keybit);
	__set_bit(BTN_MIDDLE, idev->keybit);

	/* Direct button to key mapping
	for (i = 0; i < sizeof(button_map)/sizeof(*button_map); i++) {
		set_bit(button_map[i].code, idev->keybit);
	}*/
	/* Only VKB keys
	for (i = 0; i < VKB_ROWS; i++) {
		for (j = 0; j < VKB_COLS; j++) {
			if (vkb_map_keys[i][j])
				set_bit(vkb_map_keys[i][j], idev->keybit);
		}
	}*/
	/* Enable all the keys */
	for (i = KEY_ESC; i < KEY_MICMUTE; i++) {
		set_bit(i, idev->keybit);
	}

	input_set_capability(idev, EV_MSC, MSC_SCAN);

	n3ds_input_dev->pdev = pdev;
	n3ds_input_dev->hid_input = hid_input;
	vkb_init(&n3ds_input_dev->vkb);

	error = input_register_polled_device(pdev);
	if (error) {
		printk(KERN_ERR "nintendo3ds_input.c: Failed to register device\n");
		goto err_free_dev;
	}

	return 0;

err_free_dev:
	input_free_polled_device(pdev);
err_alloc_pdev:
	iounmap(hid_input);
	release_mem_region(NINTENDO3DS_REG_HID, NINTENDO3DS_REG_HID_SIZE);
err_hidmem:
	kfree(n3ds_input_dev);
err_alloc_n3ds_input_dev:
	return error;
}

static int nintendo3ds_input_remove(struct platform_device *plat_pdev)
{
	struct nintendo3ds_input_dev *dev = platform_get_drvdata(plat_pdev);

	input_unregister_polled_device(dev->pdev);
	input_free_polled_device(dev->pdev);

	iounmap(dev->hid_input);
	release_mem_region(NINTENDO3DS_REG_HID, NINTENDO3DS_REG_HID_SIZE);

	vkb_free(&dev->vkb);

	kfree(dev);

	return 0;
}

static const struct of_device_id nintendo3ds_input_of_match[] = {
	{ .compatible = "nintendo3ds-input", },
	{},
};
MODULE_DEVICE_TABLE(of, nintendo3ds_input_of_match);

static struct platform_driver nintendo3ds_input_driver = {
	.probe	= nintendo3ds_input_probe,
	.remove	= nintendo3ds_input_remove,
	.driver	= {
		.name = "nintendo3ds-input",
		.owner = THIS_MODULE,
		.of_match_table = nintendo3ds_input_of_match,
	},
};

static int __init nintendo3ds_input_init_driver(void)
{
	return platform_driver_register(&nintendo3ds_input_driver);
}

static void __exit nintendo3ds_input_exit_driver(void)
{
	platform_driver_unregister(&nintendo3ds_input_driver);
}

module_init(nintendo3ds_input_init_driver);
module_exit(nintendo3ds_input_exit_driver);

MODULE_DESCRIPTION("Nintendo 3DS input driver");
MODULE_AUTHOR("Sergi Granell, <xerpi.g.12@gmail.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:nintendo3ds-input");
