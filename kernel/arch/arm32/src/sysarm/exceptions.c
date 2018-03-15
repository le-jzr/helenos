/** Activates use of high exception vectors addresses.
 *
 * "High vectors were introduced into some implementations of ARMv4 and are
 * required in ARMv6 implementations. High vectors allow the exception vector
 * locations to be moved from their normal address range 0x00000000-0x0000001C
 * at the bottom of the 32-bit address space, to an alternative address range
 * 0xFFFF0000-0xFFFF001C near the top of the address space. These alternative
 * locations are known as the high vectors.
 *
 * Prior to ARMv6, it is IMPLEMENTATION DEFINED whether the high vectors are
 * supported. When they are, a hardware configuration input selects whether
 * the normal vectors or the high vectors are to be used from
 * reset." ARM Architecture Reference Manual A2.6.11 (p. 64 in the PDF).
 *
 * ARM920T (gta02) TRM A2.3.5 (PDF p. 36) and ARM926EJ-S (icp) 2.3.2 (PDF p. 42)
 * say that armv4 an armv5 chips that we support implement this.
 */
bool sysarm_high_vectors_enable(void)
{
	uint32_t control_reg = SCTLR_read();

	/* switch on the high vectors bit */
	control_reg |= SCTLR_HIGH_VECTORS_EN_FLAG;

	SCTLR_write(control_reg);
}
