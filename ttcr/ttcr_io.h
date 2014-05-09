//
//  ttcr_io.h
//  ttcr_u
//
//  Created by Bernard Giroux on 2012-11-19.
//  Copyright (c) 2012 Bernard Giroux. All rights reserved.
//

/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __ttcr_u__ttcr_io__
#define __ttcr_u__ttcr_io__

#include <iostream>
#include <string>
#include <vector>

#include "ttcr_t.h"
#include "structs_ttcr.h"

void print_usage (std::ostream&, char *, int);
std::string parse_input(int argc, char * argv[], input_parameters &);
void get_params(const std::string &, input_parameters &);

#endif /* defined(__ttcr_u__spmrt_io__) */
