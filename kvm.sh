SMP=${2-2}

kvm -smp $SMP -m 2048 -boot c \
	-hda debian.qcow \
	-net nic -net user,hostfwd=tcp::10022-:22 \
	-serial stdio

