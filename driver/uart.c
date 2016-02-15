#include "driver/uart.h"
#include "driver/uart_register.h"
#include "at.h"
#include "ets_sys.h"
#include "osapi.h"

extern UartDevice    UartDev;

void ICACHE_FLASH_ATTR
uart_init(UartBautRate baund_rate)
{
    UartDev.baut_rate = baund_rate;
    uart_config(UART0);

    ETS_UART_INTR_ENABLE();
}

LOCAL void ICACHE_FLASH_ATTR
uart_config()
{
    // rcv_buff size if 0x100
	ETS_UART_INTR_ATTACH(uart_rx_intr_handler, & (UartDev.rcv_buff));
	PIN_PULLUP_DIS(PERIPHS_IO_MUX_U0TXD_U);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD);

    uart_div_modify(UART0, UART_CLK_FREQ / (UartDev.baut_rate));											// SET BAUDRATE

    WRITE_PERI_REG(UART_CONF0(UART0), ((UartDev.exist_parity & UART_PARITY_EN_M)  << UART_PARITY_EN_S) |    // SET BIT AND PARITY MODE
                                      ((UartDev.parity       & UART_PARITY_M)     << UART_PARITY_S ) |
                                      ((UartDev.stop_bits    & UART_STOP_BIT_NUM) << UART_STOP_BIT_NUM_S) |
                                      ((UartDev.data_bits    & UART_BIT_NUM)      << UART_BIT_NUM_S));
    //clear rx and tx fifo, not ready
    SET_PERI_REG_MASK(UART_CONF0(UART0), UART_RXFIFO_RST | UART_TXFIFO_RST);                                // RESET FIFO
    CLEAR_PERI_REG_MASK(UART_CONF0(UART0), UART_RXFIFO_RST | UART_TXFIFO_RST);
	//set rx fifo trigger
	WRITE_PERI_REG(UART_CONF1(UART0), ((255  & UART_RXFIFO_FULL_THRHD)  << UART_RXFIFO_FULL_THRHD_S)  |
									   (0x02   & UART_RX_TOUT_THRHD)      << UART_RX_TOUT_THRHD_S       |
									  ((0x10   & UART_TXFIFO_EMPTY_THRHD) << UART_TXFIFO_EMPTY_THRHD_S) | UART_RX_TOUT_EN);

	SET_PERI_REG_MASK(UART_INT_ENA(UART0), UART_RXFIFO_TOUT_INT_ENA | UART_FRM_ERR_INT_ENA);
    //clear all interrupt
    WRITE_PERI_REG(UART_INT_CLR(UART0), 0xffff);
    //enable rx_interrupt
    SET_PERI_REG_MASK(UART_INT_ENA(UART0), UART_RXFIFO_FULL_INT_ENA | UART_RXFIFO_OVF_INT_ENA);
}

LOCAL void
uart_rx_intr_handler(void *pt)
{
    if (UART_FRM_ERR_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_FRM_ERR_INT_ST)) {

        WRITE_PERI_REG(UART_INT_CLR(UART0), UART_FRM_ERR_INT_CLR);
    } else if (UART_RXFIFO_FULL_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_FULL_INT_ST)) {
    	ETS_UART_INTR_DISABLE();
        WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR);
        system_os_post(AT_RECV_TASK, 0, 0);
//        os_printf("data: %d\n", size);
//        while(uart_getchar()){};

    } else if (UART_RXFIFO_TOUT_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_TOUT_INT_ST)) {
    	ETS_UART_INTR_DISABLE();
        WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_TOUT_INT_CLR);
        system_os_post(AT_RECV_TASK, 0, 0);
//        os_printf("text: %d\n", size);
//        while(uart_getchar()){};

    } else if (UART_TXFIFO_EMPTY_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_TXFIFO_EMPTY_INT_ST)) {
    	CLEAR_PERI_REG_MASK(UART_INT_ENA(UART0), UART_TXFIFO_EMPTY_INT_ENA);
        WRITE_PERI_REG(UART_INT_CLR(UART0), UART_TXFIFO_EMPTY_INT_CLR);
    }else if(UART_RXFIFO_OVF_INT_ST  == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_OVF_INT_ST)){
        WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_OVF_INT_CLR);
    }
}

uint8 ICACHE_FLASH_ATTR
uart_getchar()
{
	uint8 ch = 0;

	if (READ_PERI_REG(UART_STATUS(UART0)) & (UART_RXFIFO_CNT << UART_RXFIFO_CNT_S)) {
		WRITE_PERI_REG(0X60000914, 0x73);
		ch = READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;
//		os_printf("%c", ch);
	}

	return ch;
}
