/*
    bbs100 3.0 WJ105
    Copyright (C) 2005  Walter de Jong <walter@heiho.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
/*
	state_sysop.h	WJ99
*/

#ifndef STATE_SYSOP_H_WJ99
#define STATE_SYSOP_H_WJ99 1

#include "User.h"

#define STATE_SYSOP_MENU				state_sysop_menu
#define STATE_CATEGORIES_MENU			state_categories_menu
#define STATE_ADD_CATEGORY				state_add_category
#define STATE_REMOVE_CATEGORY			state_remove_category
#define STATE_DISCONNECT_USER			state_disconnect_user
#define STATE_NUKE_USER					state_nuke_user
#define STATE_NUKE_YESNO				state_nuke_yesno
#define STATE_BANISH_USER				state_banish_user
#define STATE_ADD_WRAPPER				state_add_wrapper
#define STATE_EDIT_WRAPPER				state_edit_wrapper
#define STATE_IPADDR_WRAPPER			state_ipaddr_wrapper
#define STATE_IPMASK_WRAPPER			state_ipmask_wrapper
#define STATE_COMMENT_WRAPPER			state_comment_wrapper
#define STATE_CREATE_ROOM				state_create_room
#define STATE_DELETE_ROOM_NAME			state_delete_room_name
#define STATE_UNCACHE_FILE				state_uncache_file
#define STATE_MALLOC_STATUS				state_malloc_status
#define STATE_PARAMETERS_MENU			state_parameters_menu
#define STATE_SU_PASSWD					state_su_passwd
#define STATE_CHANGE_SU_PASSWD			state_change_su_passwd
#define STATE_REBOOT_TIME				state_reboot_time
#define STATE_REBOOT_PASSWORD			state_reboot_password
#define STATE_SHUTDOWN_TIME				state_shutdown_time
#define STATE_SHUTDOWN_PASSWORD			state_shutdown_password

#define STATE_SYSTEM_CONFIG_MENU		state_system_config_menu
#define STATE_PARAM_BBS_NAME			state_param_bbs_name
#define STATE_PARAM_BIND_ADDRESS		state_param_bind_address
#define STATE_PARAM_PORT_NUMBER			state_param_port_number
#define STATE_PARAM_FILE				state_param_file
#define STATE_PARAM_BASEDIR				state_param_basedir
#define STATE_PARAM_BINDIR				state_param_bindir
#define STATE_PARAM_CONFDIR				state_param_confdir
#define STATE_PARAM_FEELINGSDIR			state_param_feelingsdir
#define STATE_PARAM_ZONEINFODIR			state_param_zoneinfodir
#define STATE_PARAM_USERDIR				state_param_userdir
#define STATE_PARAM_ROOMDIR				state_param_roomdir
#define STATE_PARAM_TRASHDIR			state_param_trashdir
#define STATE_PARAM_UMASK				state_param_umask
#define STATE_PARAM_PROGRAM_MAIN		state_param_program_main
#define STATE_PARAM_PROGRAM_RESOLVER	state_param_program_resolver
#define STATE_PARAM_NAME_SYSOP			state_param_name_sysop
#define STATE_PARAM_NAME_ROOMAIDE		state_param_name_roomaide
#define STATE_PARAM_NAME_HELPER			state_param_name_helper
#define STATE_PARAM_NAME_GUEST			state_param_name_guest

#define STATE_CONFIG_FILES_MENU			state_config_files_menu
#define STATE_PARAM_GPL_SCREEN			state_param_gpl_screen
#define STATE_PARAM_MODS_SCREEN			state_param_mods_screen
#define STATE_PARAM_LOGIN_SCREEN		state_param_login_screen
#define STATE_PARAM_LOGOUT_SCREEN		state_param_logout_screen
#define STATE_PARAM_NOLOGIN_SCREEN		state_param_nologin_screen
#define STATE_PARAM_MOTD_SCREEN			state_param_motd_screen
#define STATE_PARAM_REBOOT_SCREEN		state_param_reboot_screen
#define STATE_PARAM_SHUTDOWN_SCREEN		state_param_shutdown_screen
#define STATE_PARAM_CRASH_SCREEN		state_param_crash_screen
#define STATE_PARAM_FIRST_LOGIN			state_param_first_login
#define STATE_PARAM_CREDITS_SCREEN		state_param_credits_screen
#define STATE_PARAM_HELP_STD			state_param_help_std
#define STATE_PARAM_HELP_CONFIG			state_param_help_config
#define STATE_PARAM_HELP_ROOMCONFIG		state_param_help_roomconfig
#define STATE_PARAM_HELP_SYSOP			state_param_help_sysop
#define STATE_PARAM_HOSTS_ACCESS		state_param_hosts_access
#define STATE_PARAM_BANISHED_FILE		state_param_banished_file
#define STATE_PARAM_STAT_FILE			state_param_stat_file
#define STATE_PARAM_SU_PASSWD_FILE		state_param_su_passwd_file
#define STATE_PARAM_PID_FILE			state_param_pid_file
#define STATE_PARAM_SYMTAB_FILE			state_param_symtab_file
#define STATE_PARAM_HOSTMAP_FILE		state_param_hostmap_file
#define STATE_PARAM_DEF_TIMEZONE		state_param_def_timezone

#define STATE_FEATURES_MENU				state_features_menu
#define STATE_RELOAD_FILES_MENU			state_reload_files_menu
#define STATE_DOWNLOAD_FILES_MENU		state_download_files_menu
#define STATE_UPLOAD_FILES_MENU			state_upload_files_menu
#define STATE_UPLOAD_YESNO				state_upload_yesno

#define STATE_MAXIMUMS_MENU				state_maximums_menu
#define STATE_PARAM_CACHED				state_param_cached
#define STATE_PARAM_MESSAGES			state_param_messages
#define STATE_PARAM_MAIL_MSGS			state_param_mail_msgs
#define STATE_PARAM_XMSG_LINES			state_param_xmsg_lines
#define STATE_PARAM_MSG_LINES			state_param_msg_lines
#define STATE_PARAM_CHAT_HISTORY		state_param_chat_history
#define STATE_PARAM_HISTORY				state_param_history
#define STATE_PARAM_FRIEND				state_param_friend
#define STATE_PARAM_ENEMY				state_param_enemy
#define STATE_PARAM_IDLE				state_param_idle
#define STATE_PARAM_LOCK				state_param_lock
#define STATE_PARAM_SAVE				state_param_save
#define STATE_PARAM_CACHE_TIMEOUT		state_param_cache_timeout
#define STATE_PARAM_HELPER_AGE			state_param_helper_age

#define STATE_STRINGS_MENU				state_strings_menu
#define STATE_PARAM_NAME_SYSOP			state_param_name_sysop
#define STATE_PARAM_NAME_ROOMAIDE		state_param_name_roomaide
#define STATE_PARAM_NAME_HELPER			state_param_name_helper
#define STATE_PARAM_NAME_GUEST			state_param_name_guest
#define STATE_PARAM_NOTIFY_LOGIN		state_param_notify_login
#define STATE_PARAM_NOTIFY_LOGOUT		state_param_notify_logout
#define STATE_PARAM_NOTIFY_LINKDEAD		state_param_notify_linkdead
#define STATE_PARAM_NOTIFY_IDLE			state_param_notify_idle
#define STATE_PARAM_NOTIFY_LOCKED		state_param_notify_locked
#define STATE_PARAM_NOTIFY_UNLOCKED		state_param_notify_unlocked
#define STATE_PARAM_NOTIFY_HOLD			state_param_notify_hold
#define STATE_PARAM_NOTIFY_UNHOLD		state_param_notify_unhold
#define STATE_PARAM_NOTIFY_ENTER_CHAT	state_param_notify_enter_chat
#define STATE_PARAM_NOTIFY_LEAVE_CHAT	state_param_notify_leave_chat

#define STATE_LOG_MENU					state_log_menu
#define STATE_PARAM_SYSLOG				state_param_syslog
#define STATE_PARAM_AUTHLOG				state_param_authlog
#define STATE_PARAM_ARCHIVEDIR			state_param_archivedir
#define STATE_PARAM_CRASHDIR			state_param_crashdir

void state_sysop_menu(User *, char);
void state_categories_menu(User *, char);
void state_add_category(User *, char);
void state_remove_category(User *, char);
void state_disconnect_user(User *, char);
void state_nuke_user(User *, char);
void state_nuke_yesno(User *, char);
void state_banish_user(User *, char);
void state_add_wrapper(User *, char);
void state_edit_wrapper(User *, char);
void state_ipaddr_wrapper(User *, char);
void state_ipmask_wrapper(User *, char);
void state_comment_wrapper(User *, char);
void state_create_room(User *, char);
void state_delete_room_name(User *, char);
void state_uncache_file(User *, char);
void state_malloc_status(User *, char);
void state_parameters_menu(User *, char);
void state_su_passwd(User *, char);
void state_change_su_passwd(User *, char);
void state_reboot_time(User *, char);
void state_reboot_password(User *, char);
void state_shutdown_time(User *, char);
void state_shutdown_password(User *, char);

void state_system_config_menu(User *, char);
void state_param_bbs_name(User *, char);
void state_param_bind_address(User *, char);
void state_param_port_number(User *, char);
void state_param_file(User *, char);
void state_param_basedir(User *, char);
void state_param_bindir(User *, char);
void state_param_confdir(User *, char);
void state_param_feelingsdir(User *, char);
void state_param_zoneinfodir(User *, char);
void state_param_userdir(User *, char);
void state_param_roomdir(User *, char);
void state_param_trashdir(User *, char);
void state_param_umask(User *, char);
void state_param_program_main(User *, char);
void state_param_program_resolver(User *, char);

void state_config_files_menu(User *, char);
void state_param_gpl_screen(User *, char);
void state_param_mods_screen(User *, char);
void state_param_login_screen(User *, char);
void state_param_logout_screen(User *, char);
void state_param_nologin_screen(User *, char);
void state_param_motd_screen(User *, char);
void state_param_reboot_screen(User *, char);
void state_param_shutdown_screen(User *, char);
void state_param_crash_screen(User *, char);
void state_param_first_login(User *, char);
void state_param_credits_screen(User *, char);
void state_param_help_std(User *, char);
void state_param_help_config(User *, char);
void state_param_help_roomconfig(User *, char);
void state_param_help_sysop(User *, char);
void state_param_hosts_access(User *, char);
void state_param_banished_file(User *, char);
void state_param_stat_file(User *, char);
void state_param_su_passwd_file(User *, char);
void state_param_license_file(User *, char);
void state_param_pid_file(User *, char);
void state_param_symtab_file(User *, char);
void state_param_hostmap_file(User *, char);
void state_param_def_timezone(User *, char);

void state_features_menu(User *, char);
void state_reload_files_menu(User *, char);
void state_download_files_menu(User *, char);
void download_file(User *, char *, char *);
void state_upload_files_menu(User *, char);
void upload_file(User *, char *, char *);
void state_upload_yesno(User *, char);
void upload_save(User *, char);
void upload_abort(User *, char);

void state_maximums_menu(User *, char);
void state_param_cached(User *, char);
void state_param_messages(User *, char);
void state_param_mail_msgs(User *, char);
void state_param_xmsg_lines(User *, char);
void state_param_msg_lines(User *, char);
void state_param_chat_history(User *, char);
void state_param_history(User *, char);
void state_param_friend(User *, char);
void state_param_enemy(User *, char);
void state_param_idle(User *, char);
void state_param_lock(User *, char);
void state_param_save(User *, char);
void state_param_cache_timeout(User *, char);
void state_param_helper_age(User *, char);

void state_strings_menu(User *, char);
void state_param_name_sysop(User *, char);
void state_param_name_roomaide(User *, char);
void state_param_name_helper(User *, char);
void state_param_name_guest(User *, char);
void state_param_notify_login(User *, char);
void state_param_notify_logout(User *, char);
void state_param_notify_linkdead(User *, char);
void state_param_notify_idle(User *, char);
void state_param_notify_locked(User *, char);
void state_param_notify_unlocked(User *, char);
void state_param_notify_hold(User *, char);
void state_param_notify_unhold(User *, char);
void state_param_notify_enter_chat(User *, char);
void state_param_notify_leave_chat(User *, char);

void state_log_menu(User *, char);
void state_param_syslog(User *, char);
void state_param_authlog(User *, char);
void state_param_archivedir(User *, char);
void state_param_crashdir(User *, char);

#endif	/* STATE_SYSOP_H_WJ99 */

/* EOB */
