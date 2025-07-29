SUMMARY = "bitbake-layers recipe"
DESCRIPTION = "Linux memory monitor for user space"
LICENSE = "GPL-2.0-only"
SRC_URI = "file://lmm_qznusnih.c \
           file://Makefile"

inherit module

MODULE_NAME = "lmm_qznusnih"
