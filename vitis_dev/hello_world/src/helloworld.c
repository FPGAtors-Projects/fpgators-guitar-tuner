/******************************************************************************
* Copyright (C) 2023 Advanced Micro Devices, Inc. All Rights Reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/
/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xiic.h"
#include "sleep.h"
#include "xparameters.h"

#define IIC_BASEADDR    XPAR_XIIC_0_BASEADDR
#define CODEC_ADDR      0x1A

XIic IicInstance;


int SSM2603_WriteReg(u8 RegAddr, u16 RegData)
{
    u8 Buffer[2];
    unsigned SentCount;

    Buffer[0] = (u8)((RegAddr << 1) | ((RegData >> 8) & 0x01));
    Buffer[1] = (u8)(RegData & 0xFF);

    SentCount = XIic_Send(IicInstance.BaseAddress, CODEC_ADDR,
                          Buffer, 2, XIIC_STOP);
    if (SentCount != 2) {
        return XST_FAILURE;
    }
    return XST_SUCCESS;
}

int main()
{
     XIic_Config *ConfigPtr;
    int Status;

    ConfigPtr = XIic_LookupConfig(IIC_BASEADDR);
    if (ConfigPtr == NULL) {
        xil_printf("IIC LookupConfig failed\r\n");
        return XST_FAILURE;
    }

    Status = XIic_CfgInitialize(&IicInstance, ConfigPtr, ConfigPtr->BaseAddress);
    if (Status != XST_SUCCESS) {
        xil_printf("IIC CfgInitialize failed\r\n");
        return XST_FAILURE;
    }

    Status = XIic_Start(&IicInstance);
    if (Status != XST_SUCCESS) {
        xil_printf("IIC Start failed\r\n");
        return XST_FAILURE;
    }

    xil_printf("AXI IIC initialized successfully\r\n");

    Status = SSM2603_WriteReg(0x0F, 0x000);
    if (Status != XST_SUCCESS) {
        xil_printf("SSM2603 reset write failed\r\n");
        return XST_FAILURE;
    }
    xil_printf("SSM2603 reset sent\r\n");
    
    Status = SSM2603_WriteReg(0x06, 0x000);
    if (Status != XST_SUCCESS) {
        xil_printf("SSM2603 power-up write failed\r\n");
        return XST_FAILURE;
    }
    xil_printf("SSM2603 power-up sent\r\n");

    Status = SSM2603_WriteReg(0x04, 0x010);
    if (Status != XST_SUCCESS) {
        xil_printf("SSM2603 analog path write failed\r\n");
        return XST_FAILURE;
    }
    xil_printf("SSM2603 analog audio path configured\r\n");
    
    Status = SSM2603_WriteReg(0x05, 0x000);
    if (Status != XST_SUCCESS) {
        xil_printf("SSM2603 digital path write failed\r\n");
        return XST_FAILURE;
    }
    xil_printf("SSM2603 digital audio path configured\r\n");

    Status = SSM2603_WriteReg(0x07, 0x002);
    if (Status != XST_SUCCESS) {
        xil_printf("SSM2603 interface format write failed\r\n");
        return XST_FAILURE;
    }
    xil_printf("SSM2603 digital interface format configured\r\n");

    Status = SSM2603_WriteReg(0x08, 0x000);
    if (Status != XST_SUCCESS) {
        xil_printf("SSM2603 sample rate write failed\r\n");
        return XST_FAILURE;
    }
    xil_printf("SSM2603 sample rate configured\r\n");
    
    Status = SSM2603_WriteReg(0x09, 0x001);
    if (Status != XST_SUCCESS) {
        xil_printf("SSM2603 activate write failed\r\n");
        return XST_FAILURE;
    }
    xil_printf("SSM2603 activated\r\n");

    while (1) {
    }

    return XST_SUCCESS;
}
