ifeq ($(CONFIG_BUILD_ARM64_DT_OVERLAY),y)
dtbo-$(CONFIG_ARCH_HOLI) += holi-oscar-21707-overlay.dtbo
dtbo-$(CONFIG_ARCH_HOLI) += holi-oscar-21708-overlay.dtbo
dtbo-$(CONFIG_ARCH_HOLI) += holi-oscar-21629-overlay.dtbo
dtbo-$(CONFIG_ARCH_HOLI) += holi-oscar-2162B-overlay.dtbo
dtbo-$(CONFIG_ARCH_HOLI) += holi-oscar-216E9-overlay.dtbo
dtbo-$(CONFIG_ARCH_HOLI) += holi-oscar-216EA-overlay.dtbo

holi-oscar-21707-overlay.dtbo-base := holi-oscar.dtb
holi-oscar-21708-overlay.dtbo-base := holi-oscar.dtb
holi-oscar-21629-overlay.dtbo-base := holi-oscar.dtb
holi-oscar-2162B-overlay.dtbo-base := holi-oscar.dtb
holi-oscar-216E9-overlay.dtbo-base := holi-oscar.dtb
holi-oscar-216EA-overlay.dtbo-base := holi-oscar.dtb
endif

always := $(dtb-y)
subdir-y := $(dts-dirs)
clean-files := *.dtb *.dtbo
