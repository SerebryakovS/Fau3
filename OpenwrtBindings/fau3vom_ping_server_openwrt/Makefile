include $(TOPDIR)/rules.mk

PKG_NAME:=fau3vom
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/fau3vom
	SECTION:=utils
	CATEGORY:=Utilities
	TITLE:=Fau3vom
	DEPENDS:=+libwebsockets +libjson-c +libhiredis
endef

define Package/fau3vom/description
	Fau3vom server
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Configure
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
	TARGET_CC=$(TARGET_CC) STAGING_DIR=$(STAGING_DIR) \
	CFLAGS="$(TARGET_CFLAGS) -I$(STAGING_DIR)/usr/include/json-c/" \
	LDFLAGS="$(TARGET_LDFLAGS) -L$(STAGING_DIR)/usr/lib/" \
	LIBS="-ljson-c -lhiredis -lwebsockets" \
	all
endef

define Package/fau3vom/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/fau3vom $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,fau3vom))
