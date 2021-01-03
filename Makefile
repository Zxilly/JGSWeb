include $(TOPDIR)/rules.mk

PKG_NAME:=JGSWeb
PKG_VERSION:=1.0
PKG_RELEASE:=10


PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

TARGET_LDFLAGS+=-lcurl

include $(INCLUDE_DIR)/package.mk

define Package/jgsweb
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Keep login status at JGSU
  URL:=https://github.com/Zxilly/JGSWeb
  DEPENDS:=+libcurl
endef

define Package/jgsweb/description
  Keep login status at JGSU.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	cp ./src/* $(PKG_BUILD_DIR)
	$(Build/Patch)
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) -o $(PKG_BUILD_DIR)/jgsweb.o -c $(PKG_BUILD_DIR)/jgsweb.c
	$(TARGET_CC) $(TARGET_LDFLAGS) -o $(PKG_BUILD_DIR)/$1 $(PKG_BUILD_DIR)/jgsweb.o
endef

define Package/jgsweb/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/jgsweb $(1)/usr/bin
endef

$(eval $(call BuildPackage,jgsweb))
