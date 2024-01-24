#!/bin/bash

# Удаляем файлы
rm -f lab2_kernel.ko
rm -f lab2_kernel.mod
rm -f lab2_kernel.mod.c
rm -f lab2_kernel.mod.o
rm -f lab2_kernel.o
rm -f modules.order
rm -f Module.symvers

# Выгружаем модуль
sudo rmmod lab2_kernel
