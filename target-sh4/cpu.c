/*
 * QEMU SuperH CPU
 *
 * Copyright (c) 2005 Samuel Tardieu
 * Copyright (c) 2012 SUSE LINUX Products GmbH
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/lgpl-2.1.html>
 */

#include "cpu.h"
#include "qemu-common.h"
#include "migration/vmstate.h"


/* CPUClass::reset() */
static void superh_cpu_reset(CPUState *s)
{
    SuperHCPU *cpu = SUPERH_CPU(s);
    SuperHCPUClass *scc = SUPERH_CPU_GET_CLASS(cpu);
    CPUSH4State *env = &cpu->env;

    if (qemu_loglevel_mask(CPU_LOG_RESET)) {
        qemu_log("CPU Reset (CPU %d)\n", s->cpu_index);
        log_cpu_state(env, 0);
    }

    scc->parent_reset(s);

    memset(env, 0, offsetof(CPUSH4State, breakpoints));
    tlb_flush(env, 1);

    env->pc = 0xA0000000;
#if defined(CONFIG_USER_ONLY)
    env->fpscr = FPSCR_PR; /* value for userspace according to the kernel */
    set_float_rounding_mode(float_round_nearest_even, &env->fp_status); /* ?! */
#else
    env->sr = SR_MD | SR_RB | SR_BL | SR_I3 | SR_I2 | SR_I1 | SR_I0;
    env->fpscr = FPSCR_DN | FPSCR_RM_ZERO; /* CPU reset value according to SH4 manual */
    set_float_rounding_mode(float_round_to_zero, &env->fp_status);
    set_flush_to_zero(1, &env->fp_status);
#endif
    set_default_nan_mode(1, &env->fp_status);
}

static void superh_cpu_realizefn(DeviceState *dev, Error **errp)
{
    SuperHCPU *cpu = SUPERH_CPU(dev);
    SuperHCPUClass *scc = SUPERH_CPU_GET_CLASS(dev);

    cpu_reset(CPU(cpu));
    qemu_init_vcpu(&cpu->env);

    scc->parent_realize(dev, errp);
}

static void superh_cpu_initfn(Object *obj)
{
    CPUState *cs = CPU(obj);
    SuperHCPU *cpu = SUPERH_CPU(obj);
    CPUSH4State *env = &cpu->env;

    cs->env_ptr = env;
    cpu_exec_init(env);

    env->movcal_backup_tail = &(env->movcal_backup);

    if (tcg_enabled()) {
        sh4_translate_init();
    }
}

static const VMStateDescription vmstate_sh_cpu = {
    .name = "cpu",
    .unmigratable = 1,
};

static void superh_cpu_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    CPUClass *cc = CPU_CLASS(oc);
    SuperHCPUClass *scc = SUPERH_CPU_CLASS(oc);

    scc->parent_realize = dc->realize;
    dc->realize = superh_cpu_realizefn;

    scc->parent_reset = cc->reset;
    cc->reset = superh_cpu_reset;

    dc->vmsd = &vmstate_sh_cpu;
}

static const TypeInfo superh_cpu_type_info = {
    .name = TYPE_SUPERH_CPU,
    .parent = TYPE_CPU,
    .instance_size = sizeof(SuperHCPU),
    .instance_init = superh_cpu_initfn,
    .abstract = false,
    .class_size = sizeof(SuperHCPUClass),
    .class_init = superh_cpu_class_init,
};

static void superh_cpu_register_types(void)
{
    type_register_static(&superh_cpu_type_info);
}

type_init(superh_cpu_register_types)
