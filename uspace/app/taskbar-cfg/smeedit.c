/*
 * Copyright (c) 2023 Jiri Svoboda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup taskbar-cfg
 * @{
 */
/** @file Start menu entry edit dialog
 */

#include <gfx/coord.h>
#include <stdio.h>
#include <stdlib.h>
#include <ui/fixed.h>
#include <ui/resource.h>
#include <ui/window.h>
#include "taskbar-cfg.h"
#include "smeedit.h"

static void wnd_close(ui_window_t *, void *);

static ui_window_cb_t window_cb = {
	.close = wnd_close
};

/** Window close button was clicked.
 *
 * @param window Window
 * @param arg Argument (tbcfg)
 */
static void wnd_close(ui_window_t *window, void *arg)
{
	smeedit_t *smee = (smeedit_t *)arg;

	(void)window;
	smeedit_destroy(smee);
}

/** Create start menu entry edit dialog.
 *
 * @param smenu Start menu
 * @param rsmee Place to store pointer to new start menu entry edit window
 * @return EOK on success or an error code
 */
errno_t smeedit_create(startmenu_t *smenu, smeedit_t **rsmee)
{
	ui_wnd_params_t params;
	ui_t *ui;
	ui_window_t *window = NULL;
	smeedit_t *smee = NULL;
	gfx_rect_t rect;
	ui_resource_t *res;
	errno_t rc;

	ui = smenu->tbarcfg->ui;

	smee = calloc(1, sizeof(smeedit_t));
	if (smee == NULL) {
		printf("Out of memory.\n");
		return ENOMEM;
	}

	ui_wnd_params_init(&params);
	params.caption = "Edit Start Menu Entry";
	if (ui_is_textmode(ui)) {
		params.rect.p0.x = 0;
		params.rect.p0.y = 0;
		params.rect.p1.x = 50;
		params.rect.p1.y = 12;
	} else {
		params.rect.p0.x = 0;
		params.rect.p0.y = 0;
		params.rect.p1.x = 370;
		params.rect.p1.y = 200;
	}

	rc = ui_window_create(ui, &params, &window);
	if (rc != EOK) {
		printf("Error creating window.\n");
		goto error;
	}

	ui_window_set_cb(window, &window_cb, (void *)smee);
	smee->window = window;

	res = ui_window_get_res(window);

	rc = ui_fixed_create(&smee->fixed);
	if (rc != EOK) {
		printf("Error creating fixed layout.\n");
		goto error;
	}

	/* Command to run label */

	rc = ui_label_create(res, "Command to run:", &smee->lcmd);
	if (rc != EOK)
		goto error;

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 3;
		rect.p0.y = 2;
		rect.p1.x = 48;
		rect.p1.y = 3;
	} else {
		rect.p0.x = 10;
		rect.p0.y = 35;
		rect.p1.x = 190;
		rect.p1.y = 50;
	}

	ui_label_set_rect(smee->lcmd, &rect);

	rc = ui_fixed_add(smee->fixed, ui_label_ctl(smee->lcmd));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	/* Command entry */

	rc = ui_entry_create(window, "foo", &smee->ecmd);
	if (rc != EOK)
		goto error;

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 3;
		rect.p0.y = 3;
		rect.p1.x = 48;
		rect.p1.y = 4;
	} else {
		rect.p0.x = 10;
		rect.p0.y = 50;
		rect.p1.x = 360;
		rect.p1.y = 75;
	}

	ui_entry_set_rect(smee->ecmd, &rect);

	rc = ui_fixed_add(smee->fixed, ui_entry_ctl(smee->ecmd));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	/* Caption label */

	rc = ui_label_create(res, "Caption:", &smee->lcaption);
	if (rc != EOK)
		goto error;

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 3;
		rect.p0.y = 5;
		rect.p1.x = 20;
		rect.p1.y = 6;
	} else {
		rect.p0.x = 10;
		rect.p0.y = 95;
		rect.p1.x = 190;
		rect.p1.y = 110;
	}

	ui_label_set_rect(smee->lcaption, &rect);

	rc = ui_fixed_add(smee->fixed, ui_label_ctl(smee->lcaption));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	/* Caption entry */

	rc = ui_entry_create(window, "bar", &smee->ecaption);
	if (rc != EOK)
		goto error;

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 3;
		rect.p0.y = 6;
		rect.p1.x = 48;
		rect.p1.y = 7;
	} else {
		rect.p0.x = 10;
		rect.p0.y = 110;
		rect.p1.x = 360;
		rect.p1.y = 135;
	}

	ui_entry_set_rect(smee->ecaption, &rect);

	rc = ui_fixed_add(smee->fixed, ui_entry_ctl(smee->ecaption));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	/* OK button */

	rc = ui_pbutton_create(res, "OK", &smee->bok);
	if (rc != EOK)
		goto error;

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 23;
		rect.p0.y = 9;
		rect.p1.x = 35;
		rect.p1.y = 10;
	} else {
		rect.p0.x = 190;
		rect.p0.y = 155;
		rect.p1.x = 270;
		rect.p1.y = 180;
	}

	ui_pbutton_set_rect(smee->bok, &rect);
	ui_pbutton_set_default(smee->bok, true);

	rc = ui_fixed_add(smee->fixed, ui_pbutton_ctl(smee->bok));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	/* Cancel button */

	rc = ui_pbutton_create(res, "Cancel", &smee->bcancel);
	if (rc != EOK)
		goto error;

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 36;
		rect.p0.y = 9;
		rect.p1.x = 48;
		rect.p1.y = 10;
	} else {
		rect.p0.x = 280;
		rect.p0.y = 155;
		rect.p1.x = 360;
		rect.p1.y = 180;
	}

	ui_pbutton_set_rect(smee->bcancel, &rect);

	rc = ui_fixed_add(smee->fixed, ui_pbutton_ctl(smee->bcancel));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	ui_window_add(window, ui_fixed_ctl(smee->fixed));

	rc = ui_window_paint(window);
	if (rc != EOK)
		goto error;

	*rsmee = smee;
	return EOK;
error:
	if (smee->window != NULL)
		ui_window_destroy(smee->window);
	free(smee);
	return rc;
}

/** Destroy Taskbar configuration window.
 *
 * @param smee Start menu entry edit dialog
 */
void smeedit_destroy(smeedit_t *smee)
{
	ui_window_destroy(smee->window);
}

/** @}
 */
