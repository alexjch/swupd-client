/*
 *   Software Updater - client side
 *
 *      Copyright © 2012-2019 Intel Corporation.
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, version 2 or later of the License.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <unistd.h>

#include "formatter_json.h"
#include "log.h"
#include "progress.h"

static bool no_progress_report = false;
static struct step step;
static progress_fn_t progress_function = NULL;
static start_fn_t start_function = NULL;
static end_fn_t end_function = NULL;

void progress_set_format(progress_fn_t progress_fn, start_fn_t start_fn, end_fn_t end_fn)
{
	progress_function = progress_fn;
	start_function = start_fn;
	end_function = end_fn;
}

void progress_init_steps(char *steps_description, int steps_in_process)
{
	step.total = steps_in_process;
	step.current = 0;
	step.description = "";
	if (start_function) {
		start_function(steps_description);
	}
}

void progress_finish_steps(char *steps_description, int status)
{
	if (end_function) {
		end_function(steps_description, status);
	}
}

void progress_set_step(unsigned int current_step, char *desc)
{
	if (current_step == 0 || step.total == 0 || current_step > step.total) {
		if (step.total == 0) {
			debug("The total number of steps is unknown. Please initialize the steps\n");
		} else {
			debug("The current step (%d) has to be greater than 0 and smaller than the total steps (%d)\n", current_step, step.total);
		}
		abort();
	}
	step.current = current_step;
	step.description = desc;
}

void progress_set_next_step(char *desc)
{
	progress_set_step((step.current) + 1, desc);
}

struct step progress_get_step(void)
{
	return step;
}

void progress_complete_step(void)
{
	/* this kind of report only make sense if using a format other than standard */
	if (progress_function) {
		progress_report(1, 1);
	}
}

void progress_report(double count, double max)
{
	static int last_percentage = -1;
	static unsigned int last_step = 0;

	if (no_progress_report) {
		return;
	}

	/* make sure we don't have a division by zero */
	if (max != 0) {

		/* Only print when the percentage changes, so a maximum of 100 times per run */
		int percentage = (int)(100 * (count / max));

		/* we should never have a percentage bigger than 100% */
		if (percentage > 100) {
			debug("Progress percentage overflow %d%% (Count: %ld, Max: %ld)\n", percentage, (long)count, (long)max);
			return;
		}

		if (percentage != last_percentage || step.current != last_step) {
			if (progress_function) {
				progress_function(step.description, step.current, step.total, percentage);
			} else {
				if (isatty(fileno(stdout))) {
					info("\t...%d%%%s", percentage, percentage != 100 ? "\r" : "\n");
				} else {
					/* if printing to a file print every percentage
					 * in its own line */
					info("\t...%d%%\n", percentage);
				}
			}
			fflush(stdout);
			last_percentage = percentage;
			last_step = step.current;
		}
	}
}

void progress_disable(bool disable)
{
	no_progress_report = disable;
}
