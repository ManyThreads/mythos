#!/bin/bash
# mcstop+release-smp-x86.sh.in COPYRIGHT FUJITSU LIMITED 2018

# IHK SMP-x86 example McKernel unload script.
# author: Balazs Gerofi <bgerofi@riken.jp>
#      Copyright (C) 2015  RIKEN AICS
# 
# This is an example script for destroying McKernel and releasing IHK resources
# Note that the script does no output anything unless an error occurs.

SCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

prefix=${SCRIPTDIR}
#BINDIR="${prefix}/bin"
SBINDIR="${prefix}/ihk/install/sbin"
ETCDIR="$prefix"
KMODDIR="${prefix}/ihk/install/kmod"

KERNDIR=""

mem=""
cpus=""
kill_in_use=""

while getopts k OPT
do
	case ${OPT} in
	k)
		kill_in_use=1
		;;
	\?) exit 1
		;;
	esac
done
if [[ "$#" -ge "$OPTIND" ]]; then
	shift $((OPTIND - 1))
	echo "Extra arguments: $@" >&2
	exit 1
fi

echo "going to kill co-kernel!!!!"
sudo sync

# No SMP module? Exit.
if ! grep ihk_smp_x86_64 /proc/modules &>/dev/null; then exit 0; fi

if [[ -f /run/systemd/system/irqbalance.service.d/mckernel.conf ]]; then
	sudo systemctl stop irqbalance 2>/dev/null
fi

# Destroy all LWK instances
if ls /dev/mcos* 1>/dev/null 2>&1; then
	for i in /dev/mcos*; do
		ind=`echo $i|cut -c10-`;

		# Are processes still using it?
		PROCS=$(lsof -t $i 2>/dev/null)
		if [[ -n "$PROCS" ]]; then
			if ((kill_in_use)); then
				kill -9 $PROCS
			else
				echo "Proesses still using $i: $PROCS" >&2
				exit 1
			fi
		fi

		# Retry when conflicting with ihkmond
		nretry=0
		until sudo ${SBINDIR}/ihkconfig 0 destroy $ind || [ $nretry -ge 4 ]; do
		    sleep 0.25
		    nretry=$[ $nretry + 1 ]
		done
		if [ $nretry -eq 4 ]; then
		    echo "error: destroying LWK instance $ind failed" >&2
		    exit 1
		fi
	done
fi

# Allow ihkmond to flush kmsg buffer
sleep 2.0

# Query IHK-SMP resources and release them
if ! sudo ${SBINDIR}/ihkconfig 0 query cpu > /dev/null; then
	echo "error: querying cpus" >&2
	exit 1
fi

cpus=`sudo ${SBINDIR}/ihkconfig 0 query cpu`
if [ "${cpus}" != "" ]; then
	if ! sudo ${SBINDIR}/ihkconfig 0 release cpu $cpus > /dev/null; then
		echo "error: releasing CPUs" >&2
		exit 1
	fi
fi

#if ! ${SBINDIR}/ihkconfig 0 query mem > /dev/null; then
#	echo "error: querying memory" >&2
#	exit 1
#fi
#
#mem=`${SBINDIR}/ihkconfig 0 query mem`
#if [ "${mem}" != "" ]; then
#	if ! ${SBINDIR}/ihkconfig 0 release mem $mem > /dev/null; then
#		echo "error: releasing memory" >&2
#		exit 1
#	fi
#fi

# Release all memory
if ! sudo ${SBINDIR}/ihkconfig 0 release mem "all" > /dev/null; then
	echo "error: releasing memory" >&2
	exit 1
fi

# Remove delegator if loaded
#if grep mcctrl /proc/modules &>/dev/null; then
	#if ! sudo rmmod mcctrl 2>/dev/null; then
		#echo "error: removing mcctrl" >&2
		#exit 1
	#fi
#fi

# Remove SMP module
if grep ihk_smp_x86_64 /proc/modules &>/dev/null; then
	if ! sudo rmmod ihk_smp_x86_64 2>/dev/null; then
		echo "error: removing ihk_smp_x86_64" >&2
		exit 1
	fi
fi

# Remove core module
if grep -E 'ihk\s' /proc/modules &>/dev/null; then
	if ! sudo rmmod ihk 2>/dev/null; then
		echo "error: removing ihk" >&2
		exit 1
	fi
fi

# Stop ihkmond
pid=`pidof ihkmond`
if [ "${pid}" != "" ]; then
    sudo kill -9 ${pid} > /dev/null 2> /dev/null
fi

# Start irqbalance with the original settings
if [ -f /run/systemd/system/irqbalance.service.d/mckernel.conf ]; then
	for f in /run/sysconfig/irqbalance_mck_affinities/proc/irq/*/smp_affinity; do
		cat "$f" | sudo tee "${f#/run/sysconfig/irqbalance_mck_affinities}" >/dev/null 2>&1
	done
	sudo rm -rf /run/systemd/system/irqbalance.service.d/mckernel.conf \
		/run/sysconfig/irqbalance_mck /run/sysconfig/irqbalance_mck_affinities
	sudo rmdir /run/systemd/system/irqbalance.service.d 2>/dev/null
	sudo systemctl daemon-reload
	sudo systemctl restart irqbalance.service
fi

# Set back default swappiness
echo 60 | sudo tee /proc/sys/vm/swappiness 1>/dev/null
