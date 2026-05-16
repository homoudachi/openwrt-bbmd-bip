include $(TOPDIR)/rules.mk

PKG_NAME:=luci-app-bbmd
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

PKG_LICENSE:=GPL-2.0-or-later
PKG_MAINTAINER:=OpenWrt BBMD Team

include $(INCLUDE_DIR)/package.mk

define Package/luci-app-bbmd
  SECTION:=luci
  CATEGORY:=LuCI
  SUBMENU:=Applications
  TITLE:=BACnet/SC BBMD daemon - LuCI web interface
  URL:=https://github.com/homoudachi/openwrt-bbmd-bip
  DEPENDS:=+luci-base +bbmd
  PKGARCH:=all
endef

define Package/luci-app-bbmd/description
  LuCI web interface for the BBMD (BACnet Broadcast Management Device) daemon.
  Provides status monitoring and configuration management
  for BACnet/SC hub, node, bridge, and telemetry modes.
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/luci-app-bbmd/install
	$(INSTALL_DIR) $(1)/www/luci-static
	$(CP) ./htdocs/luci-static/* $(1)/www/luci-static/
	$(INSTALL_DIR) $(1)/usr/share/rpcd/acl.d
	$(INSTALL_DATA) ./root/usr/share/rpcd/acl.d/* $(1)/usr/share/rpcd/acl.d/
	$(INSTALL_DIR) $(1)/usr/share/luci/menu.d
	$(INSTALL_DATA) ./root/usr/share/luci/menu.d/* $(1)/usr/share/luci/menu.d/
	$(INSTALL_DIR) $(1)/etc/uci-defaults
	$(INSTALL_BIN) ./root/etc/uci-defaults/* $(1)/etc/uci-defaults/
endef

$(eval $(call BuildPackage,luci-app-bbmd))
