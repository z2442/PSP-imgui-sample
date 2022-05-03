
BIN = psp_imgui_sample.elf
OBJS = src/main.o src/gfx.o src/pad.o \
		  src/drawing/drawing.o src/drawing/drawing_controller.o \
		  src/widgets/widget_cursor.o src/widgets/widget_gamepad.o src/widgets/widget_icon.o src/widgets/widget_misc.o
LIBS += -lpad

# Add imgui references
OBJS += lib/imgui/imgui.o lib/imgui/imgui_demo.o lib/imgui/imgui_draw.o lib/imgui/imgui_tables.o lib/imgui/imgui_widgets.o
OBJS += lib/imgui/backends/imgui_impl_pspsdk.o lib/imgui/backends/imgui_impl_pspgu.o
INCS += -Ilib/imgui -Ilib/imgui/backends
CXXFLAGS += -std=gnu++11

LIBDIR =
LDFLAGS =
LIBS= -lpspgum -lpspgu

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = ImGui-Sample

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
