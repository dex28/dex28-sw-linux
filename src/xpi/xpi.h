/*
    Project:     Albatross / XPI
  
    Module:      xpi.h
  
    Description: Albatross DSP XPI Linux device driver
  
    Copyright (c) 2002-2005 By Mikica B Kocic
    Copyright (c) 2002-2005 By IPTC Technology Communications AB
*/

#ifndef XPIDEV_H
#define XPIDEV_H

#include <linux/ioctl.h>

// Kernel aware device name
//
#define XPI_DEVICE_NAME "xpidev"

// XPI major device number
//
enum { XPI_MAJOR_NUM = 100 };

#define IOCTL_XPI_SET_CHANNEL      _IOW ( XPI_MAJOR_NUM, 0x01, int )
#define IOCTL_XPI_SET_PCM1_SOURCE  _IOW ( XPI_MAJOR_NUM, 0x02, int )
#define IOCTL_XPI_SET_PCM1_TS      _IOW ( XPI_MAJOR_NUM, 0x03, int )
#define IOCTL_XPI_FC_COMMAND       _IO  ( XPI_MAJOR_NUM, 0x04 )
#define IOCTL_XPI_GET_MCPU_FLAG    _IO  ( XPI_MAJOR_NUM, 0x05 )

#endif // XPIDEV_H
