include $(TOPDIR)/rules.mk

PKG_NAME:=bbmd
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/homoudachi/openwrt-bbmd-bip
PKG_SOURCE_DATE:=2026-05-16
PKG_SOURCE_VERSION:=PLACEHOLDER_COMMIT
PKG_MIRROR_HASH:=skip

PKG_LICENSE:=GPL-2.0-or-later
PKG_MAINTAINER:=OpenWrt BBMD Team

PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/bbmd/default
  SECTION:=net
  CATEGORY:=Network
  URL:=https://github.com/homoudachi/openwrt-bbmd-bip
  USERID:=bbmd=200:bbmd=200
endef

define Package/bbmd-openssl
  $(call Package/bbmd/default)
  TITLE:=BACnet/SC BBMD daemon (OpenSSL)
  DEPENDS:=+libubox +libblobmsg-json +libopenssl +libwebsockets-full +openssl-util
  VARIANT:=openssl
  PROVIDES:=bbmd
  DEFAULT_VARIANT:=1
endef

define Package/bbmd-mbedtls
  $(call Package/bbmd/default)
  TITLE:=BACnet/SC BBMD daemon (mbedTLS)
  DEPENDS:=+libubox +libblobmsg-json +libmbedtls +libwebsockets-mbedtls +openssl-util
  VARIANT:=mbedtls
  PROVIDES:=bbmd
endef

ifeq ($(BUILD_VARIANT),openssl)
  CMAKE_OPTIONS += -DBBMD_TLS_BACKEND=openssl
endif

ifeq ($(BUILD_VARIANT),mbedtls)
  CMAKE_OPTIONS += -DBBMD_TLS_BACKEND=mbedtls
endif

define Package/bbmd-openssl/conffiles
/etc/config/bbmd
endef

define Package/bbmd-mbedtls/conffiles
/etc/config/bbmd
endef

define Build/Prepare
	$(call Build/Prepare/Default)
	$(CP) ./files/* $(PKG_BUILD_DIR)/
endef

define Package/bbmd-openssl/install
  $(INSTALL_DIR) $(1)/usr/sbin
  $(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/bbmd $(1)/usr/sbin/
  $(INSTALL_DIR) $(1)/usr/sbin
  $(INSTALL_BIN) $(PKG_BUILD_DIR)/cert-gen.sh $(1)/usr/sbin/bbmd-cert-gen
  $(INSTALL_DIR) $(1)/etc/init.d
  $(INSTALL_BIN) $(PKG_BUILD_DIR)/bbmd.init $(1)/etc/init.d/bbmd
  $(INSTALL_DIR) $(1)/etc/config
  $(INSTALL_DATA) $(PKG_BUILD_DIR)/bbmd.config $(1)/etc/config/bbmd
endef

define Package/bbmd-mbedtls/install
  $(INSTALL_DIR) $(1)/usr/sbin
  $(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/bbmd $(1)/usr/sbin/
  $(INSTALL_DIR) $(1)/usr/sbin
  $(INSTALL_BIN) $(PKG_BUILD_DIR)/cert-gen.sh $(1)/usr/sbin/bbmd-cert-gen
  $(INSTALL_DIR) $(1)/etc/init.d
  $(INSTALL_BIN) $(PKG_BUILD_DIR)/bbmd.init $(1)/etc/init.d/bbmd
  $(INSTALL_DIR) $(1)/etc/config
  $(INSTALL_DATA) $(PKG_BUILD_DIR)/bbmd.config $(1)/etc/config/bbmd
endef

$(eval $(call BuildPackage,bbmd-openssl))
$(eval $(call BuildPackage,bbmd-mbedtls))
