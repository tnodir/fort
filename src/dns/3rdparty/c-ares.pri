# MSVC 14.28 (win7) warns C4090 on c-ares' const qualifiers
QMAKE_CFLAGS *= -wd4090

CARES_DIR = $$PWD/c-ares

INCLUDEPATH += \
    $$CARES_DIR/src/lib \
    $$CARES_DIR/src/lib/include

SOURCES += \
    $$CARES_DIR/src/lib/ares_addrinfo2hostent.c \
    $$CARES_DIR/src/lib/ares_addrinfo_localhost.c \
    $$CARES_DIR/src/lib/ares_cancel.c \
    $$CARES_DIR/src/lib/ares_close_sockets.c \
    $$CARES_DIR/src/lib/ares_conn.c \
    $$CARES_DIR/src/lib/ares_cookie.c \
    $$CARES_DIR/src/lib/ares_data.c \
    $$CARES_DIR/src/lib/ares_destroy.c \
    $$CARES_DIR/src/lib/ares_free_hostent.c \
    $$CARES_DIR/src/lib/ares_free_string.c \
    $$CARES_DIR/src/lib/ares_freeaddrinfo.c \
    $$CARES_DIR/src/lib/ares_getaddrinfo.c \
    $$CARES_DIR/src/lib/ares_getenv.c \
    $$CARES_DIR/src/lib/ares_gethostbyaddr.c \
    $$CARES_DIR/src/lib/ares_gethostbyname.c \
    $$CARES_DIR/src/lib/ares_getnameinfo.c \
    $$CARES_DIR/src/lib/ares_hosts_file.c \
    $$CARES_DIR/src/lib/ares_init.c \
    $$CARES_DIR/src/lib/ares_library_init.c \
    $$CARES_DIR/src/lib/ares_metrics.c \
    $$CARES_DIR/src/lib/ares_options.c \
    $$CARES_DIR/src/lib/ares_parse_into_addrinfo.c \
    $$CARES_DIR/src/lib/ares_process.c \
    $$CARES_DIR/src/lib/ares_qcache.c \
    $$CARES_DIR/src/lib/ares_query.c \
    $$CARES_DIR/src/lib/ares_search.c \
    $$CARES_DIR/src/lib/ares_send.c \
    $$CARES_DIR/src/lib/ares_set_socket_functions.c \
    $$CARES_DIR/src/lib/ares_socket.c \
    $$CARES_DIR/src/lib/ares_sortaddrinfo.c \
    $$CARES_DIR/src/lib/ares_strerror.c \
    $$CARES_DIR/src/lib/ares_sysconfig.c \
    $$CARES_DIR/src/lib/ares_sysconfig_files.c \
    $$CARES_DIR/src/lib/ares_sysconfig_win.c \
    $$CARES_DIR/src/lib/ares_timeout.c \
    $$CARES_DIR/src/lib/ares_update_servers.c \
    $$CARES_DIR/src/lib/ares_version.c \
    $$CARES_DIR/src/lib/dsa/ares_array.c \
    $$CARES_DIR/src/lib/dsa/ares_htable.c \
    $$CARES_DIR/src/lib/dsa/ares_htable_asvp.c \
    $$CARES_DIR/src/lib/dsa/ares_htable_dict.c \
    $$CARES_DIR/src/lib/dsa/ares_htable_strvp.c \
    $$CARES_DIR/src/lib/dsa/ares_htable_szvp.c \
    $$CARES_DIR/src/lib/dsa/ares_htable_vpstr.c \
    $$CARES_DIR/src/lib/dsa/ares_htable_vpvp.c \
    $$CARES_DIR/src/lib/dsa/ares_llist.c \
    $$CARES_DIR/src/lib/dsa/ares_slist.c \
    $$CARES_DIR/src/lib/event/ares_event_configchg.c \
    $$CARES_DIR/src/lib/event/ares_event_thread.c \
    $$CARES_DIR/src/lib/event/ares_event_win32.c \
    $$CARES_DIR/src/lib/inet_net_pton.c \
    $$CARES_DIR/src/lib/inet_ntop.c \
    $$CARES_DIR/src/lib/legacy/ares_create_query.c \
    $$CARES_DIR/src/lib/legacy/ares_expand_name.c \
    $$CARES_DIR/src/lib/legacy/ares_expand_string.c \
    $$CARES_DIR/src/lib/legacy/ares_fds.c \
    $$CARES_DIR/src/lib/legacy/ares_getsock.c \
    $$CARES_DIR/src/lib/legacy/ares_parse_a_reply.c \
    $$CARES_DIR/src/lib/legacy/ares_parse_aaaa_reply.c \
    $$CARES_DIR/src/lib/legacy/ares_parse_caa_reply.c \
    $$CARES_DIR/src/lib/legacy/ares_parse_mx_reply.c \
    $$CARES_DIR/src/lib/legacy/ares_parse_naptr_reply.c \
    $$CARES_DIR/src/lib/legacy/ares_parse_ns_reply.c \
    $$CARES_DIR/src/lib/legacy/ares_parse_ptr_reply.c \
    $$CARES_DIR/src/lib/legacy/ares_parse_soa_reply.c \
    $$CARES_DIR/src/lib/legacy/ares_parse_srv_reply.c \
    $$CARES_DIR/src/lib/legacy/ares_parse_txt_reply.c \
    $$CARES_DIR/src/lib/legacy/ares_parse_uri_reply.c \
    $$CARES_DIR/src/lib/record/ares_dns_mapping.c \
    $$CARES_DIR/src/lib/record/ares_dns_multistring.c \
    $$CARES_DIR/src/lib/record/ares_dns_name.c \
    $$CARES_DIR/src/lib/record/ares_dns_parse.c \
    $$CARES_DIR/src/lib/record/ares_dns_record.c \
    $$CARES_DIR/src/lib/record/ares_dns_write.c \
    $$CARES_DIR/src/lib/str/ares_buf.c \
    $$CARES_DIR/src/lib/str/ares_str.c \
    $$CARES_DIR/src/lib/str/ares_strsplit.c \
    $$CARES_DIR/src/lib/util/ares_iface_ips.c \
    $$CARES_DIR/src/lib/util/ares_math.c \
    $$CARES_DIR/src/lib/util/ares_rand.c \
    $$CARES_DIR/src/lib/util/ares_threads.c \
    $$CARES_DIR/src/lib/util/ares_timeval.c \
    $$CARES_DIR/src/lib/util/ares_uri.c \
    $$CARES_DIR/src/lib/windows_port.c

HEADERS += \
    $$CARES_DIR/include/ares.h \
    $$CARES_DIR/include/ares_build.h \
    $$CARES_DIR/include/ares_dns.h \
    $$CARES_DIR/include/ares_dns_record.h \
    $$CARES_DIR/include/ares_nameser.h \
    $$CARES_DIR/include/ares_version.h \
    $$CARES_DIR/src/lib/ares_conn.h \
    $$CARES_DIR/src/lib/ares_data.h \
    $$CARES_DIR/src/lib/ares_getenv.h \
    $$CARES_DIR/src/lib/ares_inet_net_pton.h \
    $$CARES_DIR/src/lib/ares_ipv6.h \
    $$CARES_DIR/src/lib/ares_private.h \
    $$CARES_DIR/src/lib/ares_setup.h \
    $$CARES_DIR/src/lib/ares_socket.h \
    $$CARES_DIR/src/lib/config-win32.h \
    $$CARES_DIR/src/lib/dsa/ares_htable.h \
    $$CARES_DIR/src/lib/dsa/ares_slist.h \
    $$CARES_DIR/src/lib/event/ares_event.h \
    $$CARES_DIR/src/lib/event/ares_event_win32.h \
    $$CARES_DIR/src/lib/include/ares_array.h \
    $$CARES_DIR/src/lib/include/ares_buf.h \
    $$CARES_DIR/src/lib/include/ares_htable_asvp.h \
    $$CARES_DIR/src/lib/include/ares_htable_dict.h \
    $$CARES_DIR/src/lib/include/ares_htable_strvp.h \
    $$CARES_DIR/src/lib/include/ares_htable_szvp.h \
    $$CARES_DIR/src/lib/include/ares_htable_vpstr.h \
    $$CARES_DIR/src/lib/include/ares_htable_vpvp.h \
    $$CARES_DIR/src/lib/include/ares_llist.h \
    $$CARES_DIR/src/lib/include/ares_mem.h \
    $$CARES_DIR/src/lib/include/ares_str.h \
    $$CARES_DIR/src/lib/record/ares_dns_multistring.h \
    $$CARES_DIR/src/lib/record/ares_dns_private.h \
    $$CARES_DIR/src/lib/str/ares_strsplit.h \
    $$CARES_DIR/src/lib/util/ares_iface_ips.h \
    $$CARES_DIR/src/lib/util/ares_math.h \
    $$CARES_DIR/src/lib/util/ares_rand.h \
    $$CARES_DIR/src/lib/util/ares_threads.h \
    $$CARES_DIR/src/lib/util/ares_time.h \
    $$CARES_DIR/src/lib/util/ares_uri.h
