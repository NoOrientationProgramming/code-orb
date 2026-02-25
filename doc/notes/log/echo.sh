#!/bin/sh

# https://en.wikipedia.org/wiki/ANSI_escape_code

#const char *red           = "\033[0;31m";
#const char *yellow        = "\033[0;33m";
#const char *cyan          = "\033[0;36m";
#const char *dColorDefault = "\033[39m";
#const char *dReset        = "\033[0m";

#2026-02-25  20:52:46.099 +0.000  ERR  process               GwMsgDispatching.cpp:133                      foo
#2026-02-25  20:52:46.099 +0.000  WRN  process               GwMsgDispatching.cpp:134                      foo
#2026-02-25  20:52:46.099 +0.000  INF  process               GwMsgDispatching.cpp:135                      foo
#2026-02-25  20:52:46.099 +0.000  DBG  process               GwMsgDispatching.cpp:136                      foo

#2026-02-25  20:52:46.099 +0.000  ERR  process               0x5d96c7699220 GwMsgDispatching.cpp:138       foo
#2026-02-25  20:52:46.099 +0.000  WRN  process               0x5d96c7699220 GwMsgDispatching.cpp:139       foo
#2026-02-25  20:52:46.099 +0.000  INF  process               0x5d96c7699220 GwMsgDispatching.cpp:140       foo
#2026-02-25  20:52:46.100 +0.000  DBG  process               0x5d96c7699220 GwMsgDispatching.cpp:141       foo

echo "\e[31m2026-02-25  20:52:46.099 +0.000  ERR  process               GwMsgDispatching.cpp:133                      foo\e[0m"
echo "\e[33m2026-02-25  20:52:46.099 +0.000  WRN  process               GwMsgDispatching.cpp:134                      foo\e[0m"
echo "\e[0m2026-02-25  20:52:46.099 +0.000  INF  process               GwMsgDispatching.cpp:135                      foo\e[0m"
echo "\e[36m2026-02-25  20:52:46.099 +0.000  DBG  process               GwMsgDispatching.cpp:136                      foo\e[0m"
echo "\e[36m2026-02-25  20:52:46.099 +0.000  COR  process               GwMsgDispatching.cpp:137                      foo\e[0m"

echo "\e[31m2026-02-25  20:52:46.099 +0.000  ERR  process               0x5d96c7699220 GwMsgDispatching.cpp:138       foo\e[0m"
echo "\e[33m2026-02-25  20:52:46.099 +0.000  WRN  process               0x5d96c7699220 GwMsgDispatching.cpp:139       foo\e[0m"
echo "\e[0m2026-02-25  20:52:46.099 +0.000  INF  process               0x5d96c7699220 GwMsgDispatching.cpp:140       foo\e[0m"
echo "\e[36m2026-02-25  20:52:46.100 +0.000  DBG  process               0x5d96c7699220 GwMsgDispatching.cpp:141       foo\e[0m"
echo "\e[36m2026-02-25  20:52:46.100 +0.000  COR  process               0x5d96c7699220 GwMsgDispatching.cpp:142       foo\e[0m"

echo

echo "\e[38:5:245m2026-02-25  20:52:46.099 +0.000  process               GwMsgDispatching.cpp:133                      \e[0m\e[38:5:160mERR\e[0m  foo"
echo "\e[38:5:245m2026-02-25  20:52:46.099 +0.000  process               GwMsgDispatching.cpp:134                      \e[0m\e[38:5:215mWRN\e[0m  foo"
echo "\e[38:5:245m2026-02-25  20:52:46.099 +0.000  process               GwMsgDispatching.cpp:135                      \e[0m\e[38:5:7mINF\e[0m  foo"
echo "\e[38:5:245m2026-02-25  20:52:46.099 +0.000  process               GwMsgDispatching.cpp:136                      \e[0m\e[38:5:30mDBG\e[0m  foo"
echo "\e[38:5:245m2026-02-25  20:52:46.099 +0.000  process               GwMsgDispatching.cpp:137                      \e[0m\e[38:5:91mCOR\e[0m  foo"

