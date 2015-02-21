/*
    bbs100 3.3 WJ107
    Copyright (C) 1997-2015  Walter de Jong <walter@heiho.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

*/
/*
	state_config.h	WJ99
*/

#ifndef STATE_CONFIG_H_WJ99
#define STATE_CONFIG_H_WJ99 1

#include "User.h"

#define STATE_CONFIG_MENU				state_config_menu
#define STATE_CONFIG_ADDRESS			state_config_address
#define STATE_CHANGE_REALNAME			state_change_realname
#define STATE_CHANGE_ADDRESS			state_change_address
#define STATE_CHANGE_STREET				state_change_street
#define STATE_CHANGE_ZIPCODE			state_change_zipcode
#define STATE_CHANGE_CITY				state_change_city
#define STATE_CHANGE_STATE				state_change_state
#define STATE_CHANGE_COUNTRY			state_change_country
#define STATE_CHANGE_PHONE				state_change_phone
#define STATE_CHANGE_EMAIL				state_change_email
#define STATE_CHANGE_WWW				state_change_www
#define STATE_CONFIG_PROFILE			state_config_profile
#define STATE_CONFIG_VANITY				state_config_vanity
#define STATE_CONFIG_DOING				state_config_doing
#define STATE_CONFIG_XMSG_HEADER		state_config_xmsg_header
#define STATE_CONFIG_REMINDER			state_config_reminder
#define STATE_QUICKLIST_PROMPT			state_quicklist_prompt
#define STATE_EDIT_QUICKLIST			state_edit_quicklist
#define STATE_CONFIG_TERMINAL			state_config_terminal
#define STATE_CONFIG_WIDTH				state_config_width
#define STATE_CONFIG_HEIGHT				state_config_height
#define STATE_CONFIG_COLORS				state_config_colors
#define STATE_CUSTOM_COLORS				state_custom_colors
#define STATE_CONFIG_SYMBOLS			state_config_symbols
#define STATE_CUSTOM_SYMBOL_COLORS		state_custom_symbol_colors
#define STATE_CHANGE_SYMBOLS			state_change_symbols
#define STATE_CONFIG_PASSWORD			state_config_password
#define STATE_CHANGE_PASSWORD			state_change_password
#define STATE_CONFIG_ANON				state_config_anon
#define STATE_CONFIG_WHO				state_config_who
#define STATE_CONFIG_OPTIONS			state_config_options
#define STATE_DEFAULT_ROOM				state_default_room
#define STATE_CONFIG_WHO_SYSOP			state_config_who_sysop
#define STATE_CONFIG_TIMEZONE			state_config_timezone
#define STATE_TZ_CONTINENT				state_tz_continent
#define STATE_TZ_CITY					state_tz_city

int help_config(User *);

void state_config_menu(User *, char);
void state_config_address(User *, char);
void state_change_realname(User *, char);
void state_change_address(User *, char);
void state_change_street(User *, char);
void state_change_zipcode(User *, char);
void state_change_city(User *, char);
void state_change_state(User *, char);
void state_change_country(User *, char);
void state_change_phone(User *, char);
void state_change_email(User *, char);
void state_change_www(User *, char);
void state_config_profile(User *, char);
void state_config_vanity(User *, char);
void state_config_doing(User *, char);
void state_config_xmsg_header(User *, char);
void state_config_reminder(User *, char);
void state_enter_forward_recipients(User *, char);
void state_forward_room(User *, char);
void state_quicklist_prompt(User *, char);
void state_edit_quicklist(User *, char);
void state_config_terminal(User *, char);
void state_config_width(User *, char);
void state_config_height(User *, char);
void config_dimensions(User *, char, int *, void (*)(User *, char));
void state_config_colors(User *, char);
void state_custom_colors(User *, char);
void state_config_symbols(User *, char);
void state_custom_symbol_colors(User *, char);
void state_change_symbols(User *, char);
void state_config_password(User *, char);
void state_change_password(User *, char);
void state_config_anon(User *, char);
void state_config_who(User *, char);
void state_config_who_sysop(User *, char);
void state_config_options(User *, char);
void state_default_room(User *, char);
void state_config_timezone(User *, char);
void state_tz_continent(User *, char);
void state_tz_city(User *, char);
void state_tz_return(User *);

void save_profile(User *, char);
void abort_profile(User *, char);

void change_config(User *, char, char **, char *);

#endif	/* STATE_CONFIG_H_WJ99 */

/* EOB */
