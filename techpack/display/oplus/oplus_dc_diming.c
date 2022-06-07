/***************************************************************
** Copyright (C),  2020,  oplus Mobile Comm Corp.,  Ltd
** File : oplus_dc_diming.c
** Description : oplus dc_diming feature
** Version : 1.0
** Date : 2020/04/15
**
** ------------------------------- Revision History: -----------
**  <author>        <data>        <version >        <desc>
**   Qianxu         2020/04/15        1.0           Build this moudle
******************************************************************/

#include "oplus_display_private_api.h"
#include "oplus_dc_diming.h"
#include "oplus_onscreenfingerprint.h"
#include "oplus_display_panel_common.h"
#include "oplus_aod.h"


int oplus_dimlayer_bl = 0;
EXPORT_SYMBOL(oplus_dimlayer_bl);
int oplus_dimlayer_bl_enabled = 0;
EXPORT_SYMBOL(oplus_dimlayer_bl_enabled);
int oplus_datadimming_v3_skip_frame = 2;
int oplus_panel_alpha = 0;
int oplus_underbrightness_alpha = 0;
EXPORT_SYMBOL(oplus_underbrightness_alpha);
static struct dsi_panel_cmd_set oplus_priv_seed_cmd_set;

extern int oplus_dimlayer_bl_on_vblank;
extern int oplus_dimlayer_bl_off_vblank;
extern int oplus_dimlayer_bl_delay;
extern int oplus_dimlayer_bl_delay_after;
extern int oplus_dimlayer_bl_enable_v2;
extern int oplus_dimlayer_bl_enable_v2_real;
extern int oplus_dimlayer_bl_enable_v3;
extern int oplus_dimlayer_bl_enable_v3_real;
extern int oplus_datadimming_vblank_count;
extern atomic_t oplus_datadimming_vblank_ref;
extern int oplus_fod_on_vblank;
extern int oplus_fod_off_vblank;
extern bool oplus_skip_datadimming_sync;
extern int oplus_dimlayer_hbm_vblank_count;
extern atomic_t oplus_dimlayer_hbm_vblank_ref;
extern int oplus_dc2_alpha;
extern int oplus_seed_backlight;
extern int oplus_panel_alpha;
extern int oplus_dimlayer_bl_enable;

extern ktime_t oplus_backlight_time;
extern int oplus_display_mode;


static struct task_struct *hbm_notify_task;
static wait_queue_head_t hbm_notify_task_wq;
static atomic_t hbm_task_task_wakeup = ATOMIC_INIT(0);
static void hbm_notify_init(void);
int hbm_delay_off = 0;
extern bool oplus_first_vid;
extern int oplus_aod_mode;

int dsi_panel_hbm_delay_off(struct dsi_panel *panel) {
	int rc = 0;

	if (!panel) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&panel->panel_lock);

	if (!dsi_panel_initialized(panel)) {
		rc = -EINVAL;
		goto error;
	}

	rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_HBM_OFF);
	if (rc) {
		pr_err("[%s] failed to send DSI_CMD_HBM_OFF cmds, rc=%d\n",
		       panel->name, rc);
	}
error:
	mutex_unlock(&panel->panel_lock);
	return rc;
}

int dsi_display_hbm_delay_off(struct dsi_display *display, int delay) {
	int rc = 0;
	if (!display || !display->panel) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&display->display_lock);

	/* enable the clk vote for CMD mode panels */
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
			DSI_CORE_CLK, DSI_CLK_ON);
	}

	usleep_range(delay*1000, delay*1000);

	rc = dsi_panel_hbm_delay_off(display->panel);
		if (rc) {
			pr_err("[%s] failed to dsi_panel_hbm_off, rc=%d\n",
			       display->name, rc);
		}

	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
	rc = dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_OFF);
	}
	mutex_unlock(&display->display_lock);
	return rc;
}

int dsi_hbm_off_delay(int delay) {
	int rc = 0;
	if (!hbm_delay_off)
		return rc;

	dsi_display_hbm_delay_off(get_main_display(), delay);

	hbm_delay_off = 0;
	return rc;
}

static int hbm_notify_worker_kthread(void *data)
{
	int ret = 0;

	while (1) {
		ret = wait_event_interruptible(hbm_notify_task_wq, atomic_read(&hbm_task_task_wakeup));
		atomic_set(&hbm_task_task_wakeup, 0);
		dsi_hbm_off_delay(23);
		if (kthread_should_stop())
			break;
	}
	return 0;
}

static void hbm_notify_init(void)
{
	if (!hbm_notify_task) {
		hbm_notify_task = kthread_create(hbm_notify_worker_kthread, NULL, "hbm_off_notify");
		init_waitqueue_head(&hbm_notify_task_wq);
		wake_up_process(hbm_notify_task);
		pr_info("[hbmnotify]  init CREATE\n");
	}
	/* pr_info("[hbmnotify] init\n"); */
}

void hbm_notify(void)
{
	if (hbm_notify_task != NULL) {
		hbm_delay_off = 1;
		oplus_first_vid = false;
		atomic_set(&hbm_task_task_wakeup, 1);
		wake_up_interruptible(&hbm_notify_task_wq);
		/* pr_info("[hbmoffnotify] notify\n"); */
	} else {
		pr_info("[hbmoffnotify] notify is NULL\n");
	}
}

static struct oplus_brightness_alpha brightness_seed_alpha_lut_dc[] = {
	{0, 0xff},
	{1, 0xfc},
	{2, 0xfb},
	{3, 0xfa},
	{4, 0xf9},
	{5, 0xf8},
	{6, 0xf7},
	{8, 0xf6},
	{10, 0xf4},
	{15, 0xf0},
	{20, 0xea},
	{30, 0xe0},
	{45, 0xd0},
	{70, 0xbc},
	{100, 0x98},
	{120, 0x80},
	{140, 0x70},
	{160, 0x58},
	{180, 0x48},
	{200, 0x30},
	{220, 0x20},
	{240, 0x10},
	{260, 0x00},
};


int sde_connector_update_backlight(struct drm_connector *connector, bool post)
{
	struct sde_connector *c_conn = to_sde_connector(connector);
	struct dsi_display *dsi_display;

	if (c_conn->connector_type != DRM_MODE_CONNECTOR_DSI) {
		return 0;
	}

	dsi_display = c_conn->display;

	if (!dsi_display || !dsi_display->panel || !dsi_display->panel->cur_mode) {
		SDE_ERROR("Invalid params(s) dsi_display %pK, panel %pK\n",
			  dsi_display,
			  ((dsi_display) ? dsi_display->panel : NULL));
		return -EINVAL;
	}

	if (!connector->state || !connector->state->crtc) {
		return 0;
	}

	if (oplus_dimlayer_bl != oplus_dimlayer_bl_enabled) {
		struct sde_connector *c_conn = to_sde_connector(connector);
		struct drm_crtc *crtc = connector->state->crtc;
		struct dsi_panel *panel = dsi_display->panel;
		int bl_lvl = dsi_display->panel->bl_config.bl_level;
		u32 current_vblank;
		int on_vblank = 0;
		int off_vblank = 0;
		int vblank = 0;
		int ret = 0;
		int vblank_get = -EINVAL;
		int on_delay = 0, on_delay_after = 0;
		int off_delay = 0, off_delay_after = 0;
		int delay = 0, delay_after = 0;

		if (sde_crtc_get_fingerprint_mode(crtc->state)) {
			oplus_dimlayer_bl_enabled = oplus_dimlayer_bl;
			goto done;
		}

		if (panel->cur_mode->timing.refresh_rate == 120) {
			if (bl_lvl < 103) {
				on_vblank = 0;
				off_vblank = 2;

			} else {
				on_vblank = 0;
				off_vblank = 0;
			}

		} else {
			if (bl_lvl < 103) {
				on_vblank = -1;
				off_vblank = 1;
				on_delay = 11000;

			} else {
				on_vblank = -1;
				off_vblank = -1;
				on_delay = 11000;
				off_delay = 11000;
			}
		}

		if (oplus_dimlayer_bl_on_vblank != INT_MAX) {
			on_vblank = oplus_dimlayer_bl_on_vblank;
		}

		if (oplus_dimlayer_bl_off_vblank != INT_MAX) {
			off_vblank = oplus_dimlayer_bl_off_vblank;
		}


		if (oplus_dimlayer_bl) {
			vblank = on_vblank;
			delay = on_delay;
			delay_after = on_delay_after;

		} else {
			vblank = off_vblank;
			delay = off_delay;
			delay_after = off_delay_after;
		}

		if (oplus_dimlayer_bl_delay >= 0) {
			delay = oplus_dimlayer_bl_delay;
		}

		if (oplus_dimlayer_bl_delay_after >= 0) {
			delay_after = oplus_dimlayer_bl_delay_after;
		}

		vblank_get = drm_crtc_vblank_get(crtc);

		if (vblank >= 0) {
			if (!post) {
				oplus_dimlayer_bl_enabled = oplus_dimlayer_bl;
				current_vblank = drm_crtc_vblank_count(crtc);
				ret = wait_event_timeout(*drm_crtc_vblank_waitqueue(crtc),
							 current_vblank != drm_crtc_vblank_count(crtc),
							 msecs_to_jiffies(34));
				current_vblank = drm_crtc_vblank_count(crtc) + vblank;

				if (delay > 0) {
					usleep_range(delay, delay + 100);
				}

				_sde_connector_update_bl_scale_(c_conn);

				if (delay_after) {
					usleep_range(delay_after, delay_after + 100);
				}

				if (vblank > 0) {
					ret = wait_event_timeout(*drm_crtc_vblank_waitqueue(crtc),
								 current_vblank == drm_crtc_vblank_count(crtc),
								 msecs_to_jiffies(17 * 3));
				}
			}

		} else {
			if (!post) {
				current_vblank = drm_crtc_vblank_count(crtc);
				ret = wait_event_timeout(*drm_crtc_vblank_waitqueue(crtc),
							 current_vblank != drm_crtc_vblank_count(crtc),
							 msecs_to_jiffies(34));

			} else {
				if (vblank < -1) {
					current_vblank = drm_crtc_vblank_count(crtc) + 1 - vblank;
					ret = wait_event_timeout(*drm_crtc_vblank_waitqueue(crtc),
								 current_vblank == drm_crtc_vblank_count(crtc),
								 msecs_to_jiffies(17 * 3));
				}

				oplus_dimlayer_bl_enabled = oplus_dimlayer_bl;

				if (delay > 0) {
					usleep_range(delay, delay + 100);
				}

				_sde_connector_update_bl_scale_(c_conn);

				if (delay_after) {
					usleep_range(delay_after, delay_after + 100);
				}
			}
		}

		if (!vblank_get) {
			drm_crtc_vblank_put(crtc);
		}
	}

	if (oplus_dimlayer_bl_enable_v2 != oplus_dimlayer_bl_enable_v2_real) {
		struct sde_connector *c_conn = to_sde_connector(connector);

		oplus_dimlayer_bl_enable_v2_real = oplus_dimlayer_bl_enable_v2;
		_sde_connector_update_bl_scale_(c_conn);
	}

	if (oplus_dimlayer_bl_enable_v3 != oplus_dimlayer_bl_enable_v3_real) {
		struct sde_connector *c_conn = to_sde_connector(connector);

		if (oplus_datadimming_v3_skip_frame > 0) {
			oplus_datadimming_v3_skip_frame--;

		} else {
			oplus_dimlayer_bl_enable_v3_real = oplus_dimlayer_bl_enable_v3;
			_sde_connector_update_bl_scale_(c_conn);
			oplus_datadimming_v3_skip_frame = 2;
		}
	}

done:

	if (post) {
		if (oplus_datadimming_vblank_count > 0) {
			oplus_datadimming_vblank_count--;

		} else {
			while (atomic_read(&oplus_datadimming_vblank_ref) > 0) {
				drm_crtc_vblank_put(connector->state->crtc);
				atomic_dec(&oplus_datadimming_vblank_ref);
			}
		}
	}

	return 0;
}
EXPORT_SYMBOL(sde_connector_update_backlight);

int sde_connector_update_hbm(struct drm_connector *connector)
{
	struct sde_connector *c_conn = to_sde_connector(connector);
	struct dsi_display *dsi_display;
	struct sde_connector_state *c_state;
	int rc = 0;
	int fingerprint_mode;

	if (!c_conn) {
		SDE_ERROR("Invalid params sde_connector null\n");
		return -EINVAL;
	}

	if (c_conn->connector_type != DRM_MODE_CONNECTOR_DSI) {
		return 0;
	}

	c_state = to_sde_connector_state(connector->state);

	dsi_display = c_conn->display;

	if (!dsi_display || !dsi_display->panel) {
		SDE_ERROR("Invalid params(s) dsi_display %pK, panel %pK\n",
			  dsi_display,
			  ((dsi_display) ? dsi_display->panel : NULL));
		return -EINVAL;
	}

	if (!c_conn->encoder || !c_conn->encoder->crtc ||
			!c_conn->encoder->crtc->state) {
		return 0;
	}

	fingerprint_mode = sde_crtc_get_fingerprint_mode(c_conn->encoder->crtc->state);

	if (OPLUS_DISPLAY_AOD_SCENE == get_oplus_display_scene()) {
		if (sde_crtc_get_fingerprint_pressed(c_conn->encoder->crtc->state)) {
			sde_crtc_set_onscreenfinger_defer_sync(c_conn->encoder->crtc->state, true);

		} else {
			sde_crtc_set_onscreenfinger_defer_sync(c_conn->encoder->crtc->state, false);
			fingerprint_mode = false;
		}

	} else {
		sde_crtc_set_onscreenfinger_defer_sync(c_conn->encoder->crtc->state, false);
	}

	if (fingerprint_mode != dsi_display->panel->is_hbm_enabled) {
		struct drm_crtc *crtc = c_conn->encoder->crtc;
		struct dsi_panel *panel = dsi_display->panel;
		int vblank = 0;
		u32 target_vblank, current_vblank;
		int ret;

		if (oplus_fod_on_vblank >= 0) {
			panel->cur_mode->priv_info->fod_on_vblank = oplus_fod_on_vblank;
		}

		if (oplus_fod_off_vblank >= 0) {
			panel->cur_mode->priv_info->fod_off_vblank = oplus_fod_off_vblank;
		}

		if (dsi_display->panel->oplus_priv.is_aod_ramless)
			hbm_notify_init();

		pr_err("OnscreenFingerprint mode: %s",
		       fingerprint_mode ? "Enter" : "Exit");

		dsi_display->panel->is_hbm_enabled = fingerprint_mode;

		if (fingerprint_mode) {
			if (!dsi_display->panel->oplus_priv.is_aod_ramless || oplus_display_mode) {
				mutex_lock(&dsi_display->panel->panel_lock);

			if (!dsi_display->panel->panel_initialized) {
				dsi_display->panel->is_hbm_enabled = false;
				pr_err("panel not initialized, failed to Enter OnscreenFingerprint\n");
				mutex_unlock(&dsi_display->panel->panel_lock);
				return 0;
			}

			dsi_display_clk_ctrl(dsi_display->dsi_clk_handle,
					     DSI_CORE_CLK, DSI_CLK_ON);

			if (oplus_seed_backlight) {
				int frame_time_us;

				frame_time_us = mult_frac(1000, 1000, panel->cur_mode->timing.refresh_rate);
				oplus_panel_process_dimming_v2(panel, panel->bl_config.bl_level, true);
				oplus_mipi_dsi_dcs_set_display_brightness(&panel->mipi_device,
								    panel->bl_config.bl_level);
				oplus_panel_process_dimming_v2_post(panel, true);
				usleep_range(frame_time_us, frame_time_us + 100);
			}

#ifdef OPLUS_FEATURE_AOD_RAMLESS
				if (dsi_display->panel->oplus_priv.is_aod_ramless) {
					ktime_t delta = ktime_sub(ktime_get(), oplus_backlight_time);
					s64 delta_us = ktime_to_us(delta);
					if (delta_us < 34000 && delta_us >= 0)
						usleep_range(34000 - delta_us, 34000 - delta_us + 100);
				}
#endif /* OPLUS_FEATURE_AOD_RAMLESS */

				if (OPLUS_DISPLAY_AOD_SCENE != get_oplus_display_scene() &&
						dsi_display->panel->bl_config.bl_level) {
					if (dsi_display->config.panel_mode != DSI_OP_VIDEO_MODE) {
						current_vblank = drm_crtc_vblank_count(crtc);
						ret = wait_event_timeout(*drm_crtc_vblank_waitqueue(crtc),
									 current_vblank != drm_crtc_vblank_count(crtc),
									 msecs_to_jiffies(17));
					}

				vblank = panel->cur_mode->priv_info->fod_on_vblank;
				target_vblank = drm_crtc_vblank_count(crtc) + vblank;
				rc = dsi_panel_tx_cmd_set(dsi_display->panel, DSI_CMD_HBM_ON);

				if (vblank) {
					ret = wait_event_timeout(*drm_crtc_vblank_waitqueue(crtc),
								 target_vblank == drm_crtc_vblank_count(crtc),
								 msecs_to_jiffies((vblank + 1) * 17));

					if (!ret) {
						pr_err("OnscreenFingerprint failed to wait vblank timeout target_vblank=%d current_vblank=%d\n",
						       target_vblank, drm_crtc_vblank_count(crtc));
					}
				}

			} else {
				rc = dsi_panel_tx_cmd_set(dsi_display->panel, DSI_CMD_AOD_HBM_ON);
			}

			dsi_display_clk_ctrl(dsi_display->dsi_clk_handle,
					     DSI_CORE_CLK, DSI_CLK_OFF);

			mutex_unlock(&dsi_display->panel->panel_lock);

			if (rc) {
				pr_err("failed to send DSI_CMD_HBM_ON cmds, rc=%d\n", rc);
				return rc;
			}
			}
		} else {
			mutex_lock(&dsi_display->panel->panel_lock);

			if (!dsi_display->panel->panel_initialized) {
				dsi_display->panel->is_hbm_enabled = true;
				pr_err("panel not initialized, failed to Exit OnscreenFingerprint\n");
				mutex_unlock(&dsi_display->panel->panel_lock);
				return 0;
			}

			current_vblank = drm_crtc_vblank_count(crtc);

			ret = wait_event_timeout(*drm_crtc_vblank_waitqueue(crtc),
						 current_vblank != drm_crtc_vblank_count(crtc),
						 msecs_to_jiffies(17));

			oplus_skip_datadimming_sync = true;
			oplus_panel_update_backlight_unlock(panel);
			oplus_skip_datadimming_sync = false;

			vblank = panel->cur_mode->priv_info->fod_off_vblank;
			target_vblank = drm_crtc_vblank_count(crtc) + vblank;

			dsi_display_clk_ctrl(dsi_display->dsi_clk_handle,
					     DSI_CORE_CLK, DSI_CLK_ON);

			if (dsi_display->config.panel_mode == DSI_OP_VIDEO_MODE) {
				panel->oplus_priv.skip_mipi_last_cmd = true;
			}

			if (dsi_display->config.panel_mode == DSI_OP_VIDEO_MODE) {
				panel->oplus_priv.skip_mipi_last_cmd = false;
			}

			if (OPLUS_DISPLAY_AOD_HBM_SCENE == get_oplus_display_scene()) {
				if (OPLUS_DISPLAY_POWER_DOZE_SUSPEND == get_oplus_display_power_status() ||
						OPLUS_DISPLAY_POWER_DOZE == get_oplus_display_power_status()) {
					rc = dsi_panel_tx_cmd_set(dsi_display->panel, DSI_CMD_AOD_HBM_OFF);
					oplus_update_aod_light_mode_unlock(panel);
					set_oplus_display_scene(OPLUS_DISPLAY_AOD_SCENE);

				} else {
					rc = dsi_panel_tx_cmd_set(dsi_display->panel, DSI_CMD_SET_NOLP);

					if (dsi_display->panel->oplus_priv.is_aod_ramless)
						hbm_notify();

					/* set nolp would exit hbm, restore when panel status on hbm */
					if (panel->bl_config.bl_level > panel->bl_config.bl_normal_max_level) {
						oplus_panel_update_backlight_unlock(panel);
					}

					if (oplus_display_get_hbm_mode()) {
						rc = dsi_panel_tx_cmd_set(dsi_display->panel, DSI_CMD_AOD_HBM_ON);
					}

					set_oplus_display_scene(OPLUS_DISPLAY_NORMAL_SCENE);
				}

			} else if (oplus_display_get_hbm_mode()) {
				/* Do nothing to skip hbm off */
			} else if (OPLUS_DISPLAY_AOD_SCENE == get_oplus_display_scene()) {
				rc = dsi_panel_tx_cmd_set(dsi_display->panel, DSI_CMD_AOD_HBM_OFF);
				oplus_update_aod_light_mode_unlock(panel);

			} else {
				if (dsi_display->panel->oplus_priv.is_aod_ramless) {
					hbm_notify();
				} else {
					rc = dsi_panel_tx_cmd_set(dsi_display->panel, DSI_CMD_HBM_OFF);
				}
			}

			dsi_display_clk_ctrl(dsi_display->dsi_clk_handle,
					     DSI_CORE_CLK, DSI_CLK_OFF);
			mutex_unlock(&dsi_display->panel->panel_lock);

			if (vblank) {
				ret = wait_event_timeout(*drm_crtc_vblank_waitqueue(crtc),
							 target_vblank == drm_crtc_vblank_count(crtc),
							 msecs_to_jiffies((vblank + 1) * 17));

				if (!ret) {
					pr_err("OnscreenFingerprint failed to wait vblank timeout target_vblank=%d current_vblank=%d\n",
					       target_vblank, drm_crtc_vblank_count(crtc));
				}
			}
		}
	}

	if (oplus_dimlayer_hbm_vblank_count > 0) {
		oplus_dimlayer_hbm_vblank_count--;

	} else {
		while (atomic_read(&oplus_dimlayer_hbm_vblank_ref) > 0) {
			drm_crtc_vblank_put(connector->state->crtc);
			atomic_dec(&oplus_dimlayer_hbm_vblank_ref);
		}
	}

	return 0;
}
EXPORT_SYMBOL(sde_connector_update_hbm);

int oplus_seed_bright_to_alpha(int brightness)
{
	int level = ARRAY_SIZE(brightness_seed_alpha_lut_dc);
	int i = 0;
	int alpha;

	if (oplus_panel_alpha) {
		return oplus_panel_alpha;
	}

	for (i = 0; i < ARRAY_SIZE(brightness_seed_alpha_lut_dc); i++) {
		if (brightness_seed_alpha_lut_dc[i].brightness >= brightness) {
			break;
		}
	}

	if (i == 0) {
		alpha = brightness_seed_alpha_lut_dc[0].alpha;
	} else if (i == level) {
		alpha = brightness_seed_alpha_lut_dc[level - 1].alpha;
	} else
		alpha = interpolate(brightness,
				    brightness_seed_alpha_lut_dc[i - 1].brightness,
				    brightness_seed_alpha_lut_dc[i].brightness,
				    brightness_seed_alpha_lut_dc[i - 1].alpha,
				    brightness_seed_alpha_lut_dc[i].alpha);

	return alpha;
}

struct dsi_panel_cmd_set *
oplus_dsi_update_seed_backlight(struct dsi_panel *panel, int brightness,
			       enum dsi_cmd_set_type type)
{
	enum dsi_cmd_set_state state;
	struct dsi_cmd_desc *cmds;
	struct dsi_cmd_desc *oplus_cmd;
	u8 *tx_buf;
	int count, rc = 0;
	int i = 0;
	int k = 0;
	int alpha = oplus_seed_bright_to_alpha(brightness);

	if (type != DSI_CMD_SEED_MODE0 &&
			type != DSI_CMD_SEED_MODE1 &&
			type != DSI_CMD_SEED_MODE2 &&
			type != DSI_CMD_SEED_MODE3 &&
			type != DSI_CMD_SEED_MODE4 &&
			type != DSI_CMD_SEED_OFF) {
		return NULL;
	}

	if (type == DSI_CMD_SEED_OFF) {
		type = DSI_CMD_SEED_MODE0;
	}

	cmds = panel->cur_mode->priv_info->cmd_sets[type].cmds;
	count = panel->cur_mode->priv_info->cmd_sets[type].count;
	state = panel->cur_mode->priv_info->cmd_sets[type].state;

	oplus_cmd = kmemdup(cmds, sizeof(*cmds) * count, GFP_KERNEL);

	if (!oplus_cmd) {
		rc = -ENOMEM;
		goto error;
	}

	for (i = 0; i < count; i++) {
		oplus_cmd[i].msg.tx_buf = NULL;
	}

	for (i = 0; i < count; i++) {
		u32 size;

		size = oplus_cmd[i].msg.tx_len * sizeof(u8);

		oplus_cmd[i].msg.tx_buf = kmemdup(cmds[i].msg.tx_buf, size, GFP_KERNEL);

		if (!oplus_cmd[i].msg.tx_buf) {
			rc = -ENOMEM;
			goto error;
		}
	}

	for (i = 0; i < count; i++) {
		if (oplus_cmd[i].msg.tx_len != 0x16) {
			continue;
		}

		tx_buf = (u8 *)oplus_cmd[i].msg.tx_buf;

		for (k = 0; k < oplus_cmd[i].msg.tx_len; k++) {
			if (k == 0) {
				continue;
			}

			tx_buf[k] = tx_buf[k] * (255 - alpha) / 255;
		}
	}

	if (oplus_priv_seed_cmd_set.cmds) {
		for (i = 0; i < oplus_priv_seed_cmd_set.count; i++) {
			kfree(oplus_priv_seed_cmd_set.cmds[i].msg.tx_buf);
		}

		kfree(oplus_priv_seed_cmd_set.cmds);
	}

	oplus_priv_seed_cmd_set.cmds = oplus_cmd;
	oplus_priv_seed_cmd_set.count = count;
	oplus_priv_seed_cmd_set.state = state;
	oplus_dc2_alpha = alpha;

	return &oplus_priv_seed_cmd_set;

error:

	if (oplus_cmd) {
		for (i = 0; i < count; i++) {
			kfree(oplus_cmd[i].msg.tx_buf);
		}

		kfree(oplus_cmd);
	}

	return ERR_PTR(rc);
}
EXPORT_SYMBOL(oplus_dsi_update_seed_backlight);

int oplus_display_panel_get_dim_alpha(void *buf)
{
	unsigned int *temp_alpha = buf;
	struct dsi_display *display = get_main_display();

	if (!display->panel->is_hbm_enabled ||
			(get_oplus_display_power_status() != OPLUS_DISPLAY_POWER_ON)) {
		(*temp_alpha) = 0;
		return 0;
	}

	(*temp_alpha) = oplus_underbrightness_alpha;
	return 0;
}

int oplus_display_panel_set_dim_alpha(void *buf)
{
	unsigned int *temp_alpha = buf;

	(*temp_alpha) = oplus_panel_alpha;

	return 0;
}

int oplus_display_panel_get_dim_dc_alpha(void *buf)
{
	int ret = 0;
	unsigned int *temp_dim_alpha = buf;
	struct dsi_display *display = get_main_display();

	if (display->panel->is_hbm_enabled ||
			get_oplus_display_power_status() != OPLUS_DISPLAY_POWER_ON) {
		ret = 0;
	}

	if (oplus_dc2_alpha != 0) {
		ret = oplus_dc2_alpha;

	} else if (oplus_underbrightness_alpha != 0) {
		ret = oplus_underbrightness_alpha;

	} else if (oplus_dimlayer_bl_enable_v3_real) {
		ret = 1;
	}

	(*temp_dim_alpha) = ret;
	return 0;
}

int oplus_display_panel_set_dimlayer_enable(void *data)
{
	struct dsi_display *display = NULL;
	struct drm_connector *dsi_connector = NULL;
	uint32_t *dimlayer_enable = data;

	display = get_main_display();
	if (!display) {
		return -EINVAL;
	}

	dsi_connector = display->drm_conn;
	if (display && display->name) {
		int enable = (*dimlayer_enable);
		int err = 0;

		mutex_lock(&display->display_lock);
		if (!dsi_connector || !dsi_connector->state || !dsi_connector->state->crtc) {
			pr_err("[%s]: display not ready\n", __func__);
		} else {
			err = drm_crtc_vblank_get(dsi_connector->state->crtc);
			if (err) {
				pr_err("failed to get crtc vblank, error=%d\n", err);
			} else {
				/* do vblank put after 7 frames */
				oplus_datadimming_vblank_count = 7;
				atomic_inc(&oplus_datadimming_vblank_ref);
			}
		}

		usleep_range(17000, 17100);
		if (!strcmp(display->panel->oplus_priv.vendor_name, "ANA6706")) {
			oplus_dimlayer_bl_enable = enable;
		} else {
			if (!strcmp(display->name, "qcom,mdss_dsi_oplus19101boe_nt37800_1080_2400_cmd"))
				oplus_dimlayer_bl_enable_v3 = enable;
			else
				oplus_dimlayer_bl_enable_v2 = enable;
		}
		mutex_unlock(&display->display_lock);
	}

	return 0;
}

int oplus_display_panel_get_dimlayer_enable(void *data)
{
	uint32_t *dimlayer_bl_enable = data;

	(*dimlayer_bl_enable) = oplus_dimlayer_bl_enable_v2;

	return 0;
}

