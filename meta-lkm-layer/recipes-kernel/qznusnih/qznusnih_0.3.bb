SUMMARY = "bitbake-layers recipe"
DESCRIPTION = "Linux memory monitor for user space"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://COPYING;md5=b234ee4d69f5fce4486a80fdaf4a4263"
SRC_URI = "file://qznusnih.c file://Makefile file://COPYING"

inherit module

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"

S = "${UNPACKDIR}"
KDIR := "/lib/modules/$(shell uname -r)/build"

IMAGE_INSTALL:append = " qznusnih"
EXTRA_OEMAKE = "KERNELDIR=${STAGING_KERNEL_DIR}"

do_install() {
    install -d ${D}/lib/modules/${KERNEL_VERSION}/extra/
    install -m 0644 qznusnih.ko ${D}/lib/modules/${KERNEL_VERSION}/extra/
}

MODULE_NAME = "qznusnih"
FILES_${PN}-dev = "/lib/modules/${KERNEL_VERSION}/extra/*.ko"
