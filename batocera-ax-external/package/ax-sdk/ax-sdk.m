EDEN_AX_VERSION = master
EDEN_AX_SITE = https://eden-emu.dev
EDEN_AX_SITE_METHOD = git
EDEN_AX_GIT_SUBMODULES = YES
EDEN_AX_LICENSE = GPL-3.0-or-later
EDEN_AX_LICENSE_FILES = LICENSE.txt
EDEN_AX_DEPENDENCIES = ax-sdk sdl2 vulkan-headers vulkan-loader ffmpeg zlib zstd

EDEN_AX_CONF_OPTS = \
	-DCMAKE_BUILD_TYPE=Release \
	-DENABLE_AX_SDK=ON \
	-DENABLE_QT=OFF \
	-DENABLE_SDL2=ON \
	-DENABLE_WEB_SERVICE=OFF \
	-DYUZU_USE_BUNDLED_VCPKG=OFF \
	-DYUZU_TESTS=OFF \
	-DCMAKE_CXX_FLAGS="$(TARGET_CXXFLAGS) -march=skylake -flto=thin"

$(eval $(cmake-package))
