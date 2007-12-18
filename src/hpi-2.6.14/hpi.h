/*
    Project:     Albatross / HPI
  
    Module:      hpi.h
  
    Description: Albatross DSP HPI Linux device driver
  
    Copyright (c) 2002-2005 By Mikica B Kocic
    Copyright (c) 2002-2005 By IPTC Technology Communications AB
*/

#ifndef HPIDEV_H
#define HPIDEV_H

#include <linux/ioctl.h>

// Kernel aware device name
//
#define HPI_DEVICE_NAME "hpidev"

// HPI major device number
//
enum { HPI_MAJOR_NUM = 100 };

#define IOCTL_HPI_RESET_DSP        _IOW ( HPI_MAJOR_NUM, 0x01, int )

#define IOCTL_HPI_GET_DSP_CHIP_ID  _IO  ( HPI_MAJOR_NUM, 0x02 )

#define IOCTL_HPI_RUN_PROGRAM      _IOW ( HPI_MAJOR_NUM, 0x03, unsigned long )

#define IOCTL_HPI_GET_HPIC         _IO  ( HPI_MAJOR_NUM, 0x04 )

#define IOCTL_HPI_SET_D_CHANNEL    _IOW ( HPI_MAJOR_NUM, 0x05, unsigned long )

#define IOCTL_HPI_SET_B_CHANNEL    _IOW ( HPI_MAJOR_NUM, 0x06, unsigned long )

#define IOCTL_HPI_GET_DSPWD_REPORT _IOR ( HPI_MAJOR_NUM, 0x07, unsigned char* )

#define IOCTL_HPI_ON_CLOSE_WRITE   _IO  ( HPI_MAJOR_NUM, 0x08 ) // Next write will contain data

#define IOCTL_HPI_GET_IODEV_COUNT  _IO  ( HPI_MAJOR_NUM, 0x09 ) // Returns number of open i/o devices

#define IOCTL_HPI_SET_CHANNEL_INFO _IO  ( HPI_MAJOR_NUM, 0x0A )

#endif // HPIDEV_H
