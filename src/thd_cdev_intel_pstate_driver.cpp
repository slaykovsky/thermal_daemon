/*
 * thd_sysfs_intel_pstate_driver.cpp: thermal cooling class implementation
 *	using Intel p state driver
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 or later as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 *
 * Author Name <Srinivas.Pandruvada@linux.intel.com>
 *
 */

#include "thd_cdev_intel_pstate_driver.h"

/*
 contents of "/sys/devices/system/cpu/intel_pstate/"
	max_perf_pct,  min_perf_pct,  no_turbo
 */

void cthd_intel_p_state_cdev::set_curr_state(int state, int arg)
{
	std::stringstream tc_state_dev;
	int new_state;

	tc_state_dev << "/max_perf_pct";
	if(cdev_sysfs.exists(tc_state_dev.str()))
	{
		std::stringstream state_str;
		if (state == 0)
			new_state = 100;
		else
		{
			new_state = 100 - (state + min_compensation) * unit_value;
		}
		state_str << new_state;
		thd_log_debug("set cdev state index %d state %d percent %d\n", index, state, new_state);
		if (cdev_sysfs.write(tc_state_dev.str(), state_str.str()) < 0)
			curr_state = (state == 0) ? 0 : max_state;
		else
			curr_state = state;
	}
	else
		curr_state = (state == 0) ? 0 : max_state;
}

int cthd_intel_p_state_cdev::get_max_state()
{
	return max_state;
}

int cthd_intel_p_state_cdev::update()
{
	std::stringstream tc_state_dev;

	tc_state_dev << "/max_perf_pct";
	if(cdev_sysfs.exists(tc_state_dev.str()))
	{
		std::string state_str;
		cdev_sysfs.read(tc_state_dev.str(), state_str);
		std::istringstream(state_str) >> curr_state;
	}
	else {
		return THD_ERROR;
	}
	highest_turbo_freq_state = msr.get_max_turbo_freq();
	if (highest_turbo_freq_state == THD_ERROR)
		return THD_ERROR;
	lowest_turbo_freq_state = msr.get_min_turbo_freq();
	if (lowest_turbo_freq_state == THD_ERROR)
		return THD_ERROR;
	lowest_freq_state = msr.get_min_freq();
	if (lowest_freq_state == THD_ERROR)
		return THD_ERROR;
	max_state = highest_turbo_freq_state;
	type_str = "intel_pstate_driver";
	min_compensation = highest_turbo_freq_state - lowest_turbo_freq_state;
	if (max_state > 0)
	{
		unit_value = 100 / max_state;
		max_offset = 100 % max_state;
	}

	curr_state = max_state - curr_state/unit_value;
	if (curr_state < 0)
		curr_state = 0;
	thd_log_debug("cooling dev index:%d, curr_state:%d, max_state:%d, unit:%f, min_com:%d, type:%s\n", index, curr_state, max_state, unit_value, min_compensation, type_str.c_str());

	return THD_SUCCESS;
}