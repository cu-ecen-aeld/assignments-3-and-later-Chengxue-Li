# Kernel Oops Analysis: `/dev/faulty` Write

## Error Summary

When running:

```sh
echo "hello_world" > /dev/faulty
```

the kernel produces an oops with the following key message:

```
Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000
Mem abort info:
  ESR = 0x0000000096000045
  EC = 0x25: DABT (current EL), IL = 32 bits
  SET = 0, FnV = 0
  EA = 0, S1PTW = 0
  FSC = 0x05: level 1 translation fault
Data abort info:
  ISV = 0, ISS = 0x00000045
  CM = 0, WnR = 1
user pgtable: 4k pages, 39-bit VAs, pgdp=0000000041bb5000
[0000000000000000] pgd=0000000000000000, p4d=0000000000000000, pud=0000000000000000
Internal error: Oops: 0000000096000045 [#1] SMP
Modules linked in: hello(O) faulty(O) scull(O)
CPU: 0 PID: 152 Comm: sh Tainted: G           O       6.1.44 #1
Hardware name: linux,dummy-virt (DT)
pstate: 80000005 (Nzcv daif -PAN -UAO -TCO -DIT -SSBS BTYPE=--)
pc : faulty_write+0x10/0x20 [faulty]
lr : vfs_write+0xc8/0x390
sp : ffffffc008e13d20
x29: ffffffc008e13d80 x28: ffffff8001b31a80 x27: 0000000000000000
x26: 0000000000000000 x25: 0000000000000000 x24: 0000000000000000
x23: 000000000000000c x22: 000000000000000c x21: ffffffc008e13dc0
x20: 000000555987bdd0 x19: ffffff8001c0bd00 x18: 0000000000000000
x17: 0000000000000000 x16: 0000000000000000 x15: 0000000000000000
x14: 0000000000000000 x13: 0000000000000000 x12: 0000000000000000
x11: 0000000000000000 x10: 0000000000000000 x9 : 0000000000000000
x8 : 0000000000000000 x7 : 0000000000000000 x6 : 0000000000000000
x5 : 0000000000000001 x4 : ffffffc000787000 x3 : ffffffc008e13dc0
x2 : 000000000000000c x1 : 0000000000000000 x0 : 0000000000000000
Call trace:
 faulty_write+0x10/0x20 [faulty]
 ksys_write+0x74/0x110
 __arm64_sys_write+0x1c/0x30
 invoke_syscall+0x54/0x130
 el0_svc_common.constprop.0+0x44/0xf0
 do_el0_svc+0x2c/0xc0
 el0_svc+0x2c/0x90
 el0t_64_sync_handler+0xf4/0x120
 el0t_64_sync+0x18c/0x190
Code: d2800001 d2800000 d503233f d50323bf (b900003f) 
---[ end trace 0000000000000000 ]---
```

## Analysis

- The oops is triggered by a NULL pointer dereference in the `faulty_write` function of the `faulty` kernel module.
- The program counter (`pc`) points to an instruction in `faulty_write` that attempts to access memory at address `0x0`.
- The call trace shows the error occurs during a write system call to `/dev/faulty`.
- The register dump confirms that several registers (notably `x0`, which is often used for the first function argument in ARM64) are `0x0` at the time of the fault.

### Cause

- The `faulty` module is intentionally designed to demonstrate a kernel bug by dereferencing a NULL pointer when its write operation is called.
- This results in a "level 1 translation fault" (page table lookup failure) because the kernel tries to access an unmapped address.

### Impact

- The kernel oops does not crash the entire system but marks the kernel as "tainted" due to the module causing the fault.
- The stack trace and register dump help developers debug the faulty code.

## References

- [Mastering Markdown](https://guides.github.com/features/mastering-markdown/)
- [Lecture discussions on kernel oops and debugging](#)

## Conclusion

This oops demonstrates the importance of careful pointer handling in kernel code. Writing to `/dev/faulty` reliably triggers a NULL pointer dereference, which is a classic example of a kernel bug for educational purposes.