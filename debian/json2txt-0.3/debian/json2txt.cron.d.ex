#
# Regular cron jobs for the json2txt package
#
0 4	* * *	root	[ -x /usr/bin/json2txt_maintenance ] && /usr/bin/json2txt_maintenance
