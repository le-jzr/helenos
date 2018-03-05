#pragma once

#if defined(MACHINE_gta02)
	#include <arch/mach/gta02/gta02.h>
	#define platform_machine_ops gta02_machine_ops
#elif defined(MACHINE_integratorcp)
	#include <arch/mach/integratorcp/integratorcp.h>
	#define platform_machine_ops icp_machine_ops
#elif defined(MACHINE_beagleboardxm)
	#include <arch/mach/beagleboardxm/beagleboardxm.h>
	#define platform_machine_ops bbxm_machine_ops
#elif defined(MACHINE_beaglebone)
	#include <arch/mach/beaglebone/beaglebone.h>
	#define platform_machine_ops bbone_machine_ops
#elif defined(MACHINE_raspberrypi)
	#include <arch/mach/raspberrypi/raspberrypi.h>
	#define platform_machine_ops raspberrypi_machine_ops
#elif defined(MACHINE_omnia)
	#include <arch/mach/turrisomnia/turrisomnia.h>
	#define platform_machine_ops omnia_machine_ops
#else
	#error Machine type not defined.
#endif