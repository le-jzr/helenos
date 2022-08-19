/*
 * Copyright (c) 2022 Jiří Zárevúcky
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup kernel_sync
 * @{
 */

/**
 * @file
 * @brief Fair spin mutex.
 */

#include <synch/fairspinmutex.h>

#include <arch/asm.h>

void fair_spin_mutex_lock(fair_spin_mutex_t *mutex)
{
	fair_spin_mutex_assert_not_owned(mutex);
	mutex->ipl = interrupts_disable();
	mutex->locked = true;
}

void fair_spin_mutex_unlock(fair_spin_mutex_t *mutex)
{
	fair_spin_mutex_assert_owned(mutex);
	mutex->locked = false;
	interrupts_restore(mutex->ipl);
}

bool fair_spin_mutex_try_lock(fair_spin_mutex_t *mutex)
{
	fair_spin_mutex_assert_not_owned(mutex);
	mutex->ipl = interrupts_disable();
	mutex->locked = true;
	return true;
}

bool fair_spin_mutex_probably_owned__(fair_spin_mutex_t *mutex)
{
	return mutex->locked;
}

bool fair_spin_mutex_probably_not_owned__(fair_spin_mutex_t *mutex)
{
	return !mutex->locked;
}

/** @}
 */
