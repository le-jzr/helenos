#pragma once

/* ARM Generic Interrupt Controller v1.0 */

typedef struct {
	ioport32_t dcr;            // Distributor Control Register
	ioport32_t ictr;           // Interrupt Controller Type Register
	ioport32_t iidr;           // Distributor Implementer Identification Register
	ioport32_t _reserved0[29];
	ioport32_t isr[32];        // Interrupt Security Registers
	ioport32_t iser[32];       // Interrupt Set-Enable Registers
	ioport32_t icer[32];       // Interrupt Clear-Enable Registers
	ioport32_t ispr[32];       // Interrupt Set-Pending Registers
	ioport32_t icpr[32];       // Interrupt Clear-Pending Registers
	ioport32_t abr[32];        // Active Bit Registers
	ioport32_t _reserved2[32];
	ioport32_t ipr[255];       // Interrupt Priority Registers
	ioport32_t _reserved3;
	ioport32_t iptr[255];      // Interrupt Processor Targets Registers
	ioport32_t _reserved4;
	ioport32_t icfr[64];       // Interrupt Configuration Registers
	ioport32_t _impl[64];      // Implementation-defined registers at offsets 0xD00-0xDFC
	ioport32_t _reserved5[64];
	ioport32_t sgir;           // Software Generated Interrupt Register
	ioport32_t _reserved6[55];
	ioport32_t ident[8];       // Identification registers
} gicv1_distributor_t;

extern volatile gicv1_distributor_t *gicv1_distributor;

#define _d gicv1_distributor

static inline void gicv1_dist_set(gicv1_distributor_t *d)
{
	_d = d;
}

static inline void gicv1_dist_enable()
{
	_d->dcr = 1;
}

static inline void gicv1_dist_disable()
{
	_d->dcr = 0;
}

static inline bool gicv1_dist_is_enabled()
{
	return (bool) _d->dcr;
}

static inline bool gicv1_dist_implements_security()
{
	return (bool) (_d->ictr & (1 << 10));
}

static inline int gicv1_dist_num_lockable_spis() {
	if (!gicv1_dist_implements_security()) {
		return 0;
	}
	return (_d->ictr >> 11) & 0b11111;
}

static inline int gicv1_dist_num_cpus()
{
	return ((_d->ictr >> 5) & 0b1111) + 1;
}

static inline int gicv1_dist_num_lines()
{
	return 32*((_d->ictr & 0b11111) + 1);
}

#undef _d

// TODO
_Static_assert(offsetof(gicv1_distributor_t, dcr)  == 0x000, "bad offset");
_Static_assert(offsetof(gicv1_distributor_t, isr)  == 0x080, "bad offset");
_Static_assert(offsetof(gicv1_distributor_t, abr)  == 0x300, "bad offset");
_Static_assert(offsetof(gicv1_distributor_t, ipr)  == 0x400, "bad offset");
_Static_assert(offsetof(gicv1_distributor_t, iptr) == 0x800, "bad offset");
_Static_assert(offsetof(gicv1_distributor_t, icfr) == 0xc00, "bad offset");
_Static_assert(offsetof(gicv1_distributor_t, sgir)  == 0xf00, "bad offset");
_Static_assert(sizeof(gicv1_distributor_t) == 0x1000, "bad size");

typedef struct {
	ioport32_t icr;    // CPU Interface Control Register
	ioport32_t pmr;    // Interrupt Priority Mask Register
	ioport32_t bpr;    // Binary Point Register
	ioport32_t iar;    // Interrupt Acknowledge Register
	ioport32_t eoir;   // End of Interrupt Register
	ioport32_t rpr;    // Running Priority Register
	ioport32_t hpir;   // Highest Pending Interrupt Register
	ioport32_t abpr;   // Aliased Binary Point Register
	ioport32_t _reserved0[8];
	ioport32_t _impl[36];
	ioport32_t _reserved1[11];
	ioport32_t iidr;   // CPU Interface Identification Register
} gicv1_cpu_interface_t;

// TODO
_Static_assert(offsetof(gicv1_cpu_interface_t, abpr) == 0x1c, "bad offset");
_Static_assert(offsetof(gicv1_cpu_interface_t, _impl) == 0x40, "bad offset");
_Static_assert(offsetof(gicv1_cpu_interface_t, _reserved1) == 0xd0, "bad offset");
_Static_assert(offsetof(gicv1_cpu_interface_t, iidr) == 0xfc, "bad offset");
_Static_assert(sizeof(gicv1_cpu_interface_t) == 0x100, "bad size");

#define GIC_SPURIOUS_INTNO 1023

