#!/bin/bash
KERNEL=${1-/home/lipari/Documents/corsi/2023-2024/ASEE/prova/build/kvm/arch/x86/boot/bzImage}
SMP=${2-2}

kvm -smp $SMP -m 1G -boot c --enable-kvm \
    -kernel $KERNEL -append "root=/dev/sda1 console=ttyS0 rw" \
    -hda debian.qcow \
    -net user,hostfwd=tcp::10022-:22 -net nic
    -serial mon:stdio

