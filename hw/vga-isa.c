/*
 * QEMU ISA VGA Emulator.
 *
 * see docs/specs/standard-vga.txt for virtual hardware specs.
 *
 * Copyright (c) 2003 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "hw.h"
#include "ui/console.h"
#include "pc.h"
#include "vga_int.h"
#include "ui/pixel_ops.h"
#include "qemu/timer.h"
#include "loader.h"

typedef struct ISAVGAState {
    ISADevice dev;
    struct VGACommonState state;
} ISAVGAState;

static void vga_reset_isa(DeviceState *dev)
{
    ISAVGAState *d = container_of(dev, ISAVGAState, dev.qdev);
    VGACommonState *s = &d->state;

    vga_common_reset(s);
}

static int vga_initfn(ISADevice *dev)
{
    ISAVGAState *d = DO_UPCAST(ISAVGAState, dev, dev);
    VGACommonState *s = &d->state;
    MemoryRegion *vga_io_memory;
    const MemoryRegionPortio *vga_ports, *vbe_ports;

    vga_common_init(s);
    s->legacy_address_space = isa_address_space(dev);
    vga_io_memory = vga_init_io(s, &vga_ports, &vbe_ports);
    isa_register_portio_list(dev, 0x3b0, vga_ports, s, "vga");
    if (vbe_ports) {
        isa_register_portio_list(dev, 0x1ce, vbe_ports, s, "vbe");
    }
    memory_region_add_subregion_overlap(isa_address_space(dev),
                                        isa_mem_base + 0x000a0000,
                                        vga_io_memory, 1);
    memory_region_set_coalescing(vga_io_memory);
    s->ds = graphic_console_init(s->update, s->invalidate,
                                 s->screen_dump, s->text_update, s);

    vga_init_vbe(s, isa_address_space(dev));
    /* ROM BIOS */
    rom_add_vga(VGABIOS_FILENAME);
    return 0;
}

static Property vga_isa_properties[] = {
    DEFINE_PROP_UINT32("vgamem_mb", ISAVGAState, state.vram_size_mb, 8),
    DEFINE_PROP_END_OF_LIST(),
};

static void vga_class_initfn(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    ISADeviceClass *ic = ISA_DEVICE_CLASS(klass);
    ic->init = vga_initfn;
    dc->reset = vga_reset_isa;
    dc->vmsd = &vmstate_vga_common;
    dc->props = vga_isa_properties;
}

static const TypeInfo vga_info = {
    .name          = "isa-vga",
    .parent        = TYPE_ISA_DEVICE,
    .instance_size = sizeof(ISAVGAState),
    .class_init    = vga_class_initfn,
};

static void vga_register_types(void)
{
    type_register_static(&vga_info);
}

type_init(vga_register_types)
